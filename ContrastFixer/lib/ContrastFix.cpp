#include "ContrastFix.h"
#include <omp.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>

void print_help(char* argv0) {
    std::printf(
R"(Использование:
  %s [--help] \
    --input <fname> \
    --output <fname> \
    --no-omp | --omp-threads <num_threads | default> \
    --coef <float, [0.0,0.5)> \
    --schedule <static / dynamic> \
    --chunk_size <0 or higher>

OpenMP:
    --no-omp: последовательная реализация
    --omp-threads default: распараллеливание, число потоков по умолчанию 8
    --omp-threads N: распараллеливание, N > 0 потоков
    --schedule <static / dynamic>: вид планирования
    --chunk_size <int> = 0: 0 — по умолчанию; > 0 — явный размер чанка

Описание:
Поддерживаются форматы P5 (Gray) и P6 (RGB).
В процессе выполнения увеличивается контрастность.
)",
        argv0, SCHEDULE_CHUNK_SIZE
    );
}

bool parse_args(int argc, char** argv, Args& a) {
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];

        if (s == "--help") {
            print_help(argv[0]);
            return true;
        } else if (s == "--input" && i + 1 < argc) {
            a.in_path = argv[++i];
        } else if (s == "--output" && i + 1 < argc) {
            a.out_path = argv[++i];
        } else if (s == "--no-omp") {
            a.is_omp = false;
            a.threads_default = false;
            a.threads = 1;
        } else if (s == "--omp-threads" && i + 1 < argc) {
            std::string v = argv[++i];
            a.is_omp = true;
            if (v == "default") {
                a.threads_default = true;
            } else {
                char* end = nullptr;
                long n = std::strtol(v.c_str(), &end, 10);
                if (!end || *end != '\0' || n <= 0) {
                    std::fprintf(stderr, "Bad --omp-threads value.\n");
                    return false;
                }
                a.threads_default = false;
                a.threads = static_cast<int>(n);
            }
        } else if (s == "--coef" && i + 1 < argc) {
            char* end = nullptr;
            double c = std::strtod(argv[++i], &end);
            if (!end || *end != '\0' || c < 0 || c >= 0.5) {
                std::fprintf(stderr, "Bad --coef value [0.0, 0.5).\n");
                return false;
            }
            a.coef = c;
        } else if (s == "--schedule" && i + 1 < argc) {
            a.schedule_kind = argv[++i];
            if (a.schedule_kind != "static" && a.schedule_kind != "dynamic") {
                std::fprintf(stderr, "Unknown --schedule.\n");
                return false;
            }
        } else if (s == "--chunk_size" && i + 1 < argc) {
            char* end = nullptr;
            int v = static_cast<int>(std::strtoll(argv[++i], &end, 10));
            if (!end || *end != '\0' || v < 0) {
                std::fprintf(stderr, "Bad --chunk_size (>= 0).\n");
                return false;
            }
            a.chunk_size = v;
        } else {
            std::fprintf(stderr, "Wrong argument: %s\n", s.c_str());
            return false;
        }
    }

    if (a.in_path.empty() || a.out_path.empty()) {
        std::fprintf(stderr, "Need --input and --output.\n");
        return false;
    }

    return true;
}

bool read_pnm(std::string& path, Image& img) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "Cannot open: %s\n", path.c_str());
        return false;
    }

    std::string magic;
    int maxv = 0;

    f >> magic;
    f >> img.width >> img.height >> maxv;

    if (maxv != 255) {
        std::fprintf(stderr, "Bad max value.\n");
        return false;
    }

    img.channels = (magic == "P6") ? 3 : 1;

    int c = f.get();
    if (c == '\r' && f.peek() == '\n') {
        f.get();
    }
    if (c == EOF) {
        std::fprintf(stderr, "Unexpected EOF after header.\n");
        return false;
    }

    size_t size = img.width * img.height * img.channels;
    img.data.resize(size);

    f.read(reinterpret_cast<char*>(img.data.data()), size);
    if (static_cast<size_t>(f.gcount()) != size) {
        std::fprintf(stderr, "Unexpected EOF in pixel data.\n");
        return false;
    }

    return true;
}

bool write_pnm(std::string& path, Image& img) {
    std::ofstream f(path, std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "Cannot open output file: %s\n", path.c_str());
        return false;
    }

    if (img.channels == 1) {
        f << "P5\n" << img.width << " " << img.height << "\n255\n";
    } else {
        f << "P6\n" << img.width << " " << img.height << "\n255\n";
    }

    f.write(reinterpret_cast<char*>(img.data.data()), img.data.size());
    return static_cast<bool>(f);
}

StretchParams calc_params_no_omp(Image& img, double coef) {
    int C = img.channels;
    size_t N = img.width * img.height;

    std::vector<std::array<uint32_t, 256>> hist(C);
    for (auto& h : hist) h.fill(0);

    if (C == 1) {
        for (size_t i = 0; i < N; ++i) hist[0][img.data[i]]++;
    } else {
        uint8_t* p = img.data.data();
        for (size_t i = 0; i < N; ++i) {
            hist[0][p[0]]++;
            hist[1][p[1]]++;
            hist[2][p[2]]++;
            p += 3;
        }
    }

    StretchParams sp;

    if (C == 3) {
        std::array<uint64_t, 256> all{};
        for (int v = 0; v < 256; ++v) {
            all[v] = hist[0][v] + hist[1][v] + hist[2][v];
        }

        uint64_t total = 0;
        for (uint64_t x : all) total += x;

        uint64_t skip = std::floor(coef * total);

        uint64_t cnt = 0;
        int lower = 0;
        for (int v = 0; v < 256; ++v) {
            if (cnt + all[v] > skip) {
                lower = v;
                break;
            }
            cnt += all[v];
        }

        cnt = 0;
        int higher = 255;
        for (int v = 255; v >= 0; --v) {
            if (cnt + all[v] > skip) {
                higher = v;
                break;
            }
            cnt += all[v];
        }

        bool eq = lower >= higher;
        for (int c = 0; c < 3; ++c) {
            sp.lower[c] = lower;
            sp.higher[c] = higher;
            sp.is_const[c] = eq;
        }

        return sp;
    }

    // Grayscale
    for (int c = 0; c < C; ++c) {
        uint64_t total = 0;
        for (uint32_t x : hist[c]) total += x;

        uint64_t skip = std::floor(coef * total);

        uint64_t cnt = 0;
        int lower = 0;
        for (int v = 0; v < 256; ++v) {
            if (cnt + hist[c][v] > skip) {
                lower = v;
                break;
            }
            cnt += hist[c][v];
        }

        cnt = 0;
        int higher = 255;
        for (int v = 255; v >= 0; --v) {
            if (cnt + hist[c][v] > skip) {
                higher = v;
                break;
            }
            cnt += hist[c][v];
        }

        sp.lower[c] = lower;
        sp.higher[c] = higher;
        sp.is_const[c] = lower >= higher;
    }

    return sp;
}

StretchParams calc_params_omp(Image& img, double coef) {
    int C = img.channels;
    size_t N = img.width * img.height;

    int nthreads = 1;
    #pragma omp parallel
    {
        #pragma omp single
        nthreads = omp_get_num_threads();
    }

    std::vector<std::vector<std::array<uint32_t, 256>>> local(
        nthreads, std::vector<std::array<uint32_t, 256>>(C)
    );
    for (auto& v : local)
        for (auto& arr : v)
            arr.fill(0);

    if (C == 1) {
        #pragma omp parallel for schedule(runtime)
        for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(N); ++i) {
            int tid = omp_get_thread_num();
            uint8_t v = img.data[i];
            local[tid][0][v]++;
        }
    } else {
        #pragma omp parallel for schedule(runtime)
        for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(N); ++i) {
            int tid = omp_get_thread_num();
            uint8_t* p = &img.data[i * 3];
            local[tid][0][p[0]]++;
            local[tid][1][p[1]]++;
            local[tid][2][p[2]]++;
        }
    }

    std::vector<std::array<uint32_t, 256>> hist(C);
    for (auto& a : hist) a.fill(0);

    for (int t = 0; t < nthreads; ++t)
        for (int c = 0; c < C; ++c)
            for (int v = 0; v < 256; ++v)
                hist[c][v] += local[t][c][v];

    StretchParams sp;

    if (C == 3) {
        std::array<uint64_t, 256> all{};
        for (int v = 0; v < 256; ++v) {
            all[v] =
                hist[0][v] +
                hist[1][v] +
                hist[2][v];
        }

        uint64_t total = 0;
        for (uint64_t x : all) total += x;

        uint64_t skip = std::floor(coef * total);

        uint64_t cnt = 0;
        int lower = 0;
        for (int v = 0; v < 256; ++v) {
            if (cnt + all[v] > skip) {
                lower = v;
                break;
            }
            cnt += all[v];
        }

        cnt = 0;
        int higher = 255;
        for (int v = 255; v >= 0; --v) {
            if (cnt + all[v] > skip) {
                higher = v;
                break;
            }
            cnt += all[v];
        }

        bool eq = lower >= higher;
        for (int c = 0; c < 3; ++c) {
            sp.lower[c] = lower;
            sp.higher[c] = higher;
            sp.is_const[c] = eq;
        }

        return sp;
    }

    // grayscale
    for (int c = 0; c < C; ++c) {
        uint64_t total = 0;
        for (uint32_t x : hist[c]) total += x;

        uint64_t skip = std::floor(coef * total);

        uint64_t cnt = 0;
        int lower = 0;
        for (int v = 0; v < 256; ++v) {
            if (cnt + hist[c][v] > skip) {
                lower = v;
                break;
            }
            cnt += hist[c][v];
        }

        cnt = 0;
        int higher = 255;
        for (int v = 255; v >= 0; --v) {
            if (cnt + hist[c][v] > skip) {
                higher = v;
                break;
            }
            cnt += hist[c][v];
        }

        sp.lower[c] = lower;
        sp.higher[c] = higher;
        sp.is_const[c] = lower >= higher;
    }

    return sp;
}

void stretch_no_omp(Image& img, StretchParams& sp) {
    int C = img.channels;
    size_t N = img.width * img.height;

    std::array<double, 3> scale{};
    for (int c = 0; c < C; ++c)
        if (!sp.is_const[c])
            scale[c] = 255.0 / (sp.higher[c] - sp.lower[c]);

    if (C == 1) {
        bool is_const = sp.is_const[0];
        int lower = sp.lower[0];
        double sc = scale[0];

        for (size_t i = 0; i < N; ++i) {
            if (!is_const) {
                int temp = std::lround((img.data[i] - lower) * sc);
                temp = (temp < 0 ? 0 : (temp > 255 ? 255 : temp));
                img.data[i] = static_cast<uint8_t>(temp);
            }
        }
        return;
    }

    // RGB
    uint8_t* p = img.data.data();
    for (size_t i = 0; i < N; ++i) {
        for (int c = 0; c < 3; ++c) {
            if (!sp.is_const[c]) {
                int temp = std::lround((p[c] - sp.lower[c]) * scale[c]);
                p[c] = static_cast<uint8_t>(temp < 0 ? 0 : (temp > 255 ? 255 : temp));
            }
        }
        p += 3;
    }
}

void stretch_omp(Image& img, StretchParams& sp) {
    int C = img.channels;
    size_t N = img.width * img.height;

    std::array<double, 3> scale{};
    for (int c = 0; c < C; ++c)
        if (!sp.is_const[c])
            scale[c] = 255.0 / (sp.higher[c] - sp.lower[c]);

    if (C == 1) {
        bool is_const = sp.is_const[0];
        int lower = sp.lower[0];
        double sc = scale[0];

        #pragma omp parallel for schedule(runtime)
        for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(N); ++i) {
            if (!is_const) {
                int temp = std::lround((img.data[i] - lower) * sc);
                img.data[i] = static_cast<uint8_t>(temp < 0 ? 0 : (temp > 255 ? 255 : temp));
            }
        }
        return;
    }

    // RGB
    #pragma omp parallel for schedule(runtime)
    for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(N); ++i) {
        uint8_t* p = &img.data[i * 3];
        for (int c = 0; c < 3; ++c) {
            if (!sp.is_const[c]) {
                int temp = std::lround((p[c] - sp.lower[c]) * scale[c]);
                p[c] = static_cast<uint8_t>(temp < 0 ? 0 : (temp > 255 ? 255 : temp));
            }
        }
    }
}