#include <array>
#include <cstdint>
#include <string>
#include <vector>

#ifndef SCHEDULE_KIND
#define SCHEDULE_KIND static
#endif

#ifndef SCHEDULE_CHUNK_SIZE
#define SCHEDULE_CHUNK_SIZE 0
#endif

void print_help(char* argv0);

struct Args {
    std::string in_path;
    std::string out_path;
    bool is_omp = false;
    bool threads_default = false;
    int threads = 1;
    double coef = 0.0;
    std::string schedule_kind;
    int chunk_size = 0;
};

bool parse_args(int argc, char** argv, Args& a);

struct Image {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<std::uint8_t> data;
};

bool read_pnm(std::string& path, Image& img);
bool write_pnm(std::string& path, Image& img);

struct StretchParams {
    std::array<int, 3>  lower   { 0,   0,   0 };
    std::array<int, 3>  higher  { 255, 255, 255 };
    std::array<bool, 3> is_const{ false, false, false };
};

StretchParams calc_params_no_omp(Image& img, double coef);
StretchParams calc_params_omp(Image& img, double coef);

void stretch_no_omp(Image& img, StretchParams& sp);
void stretch_omp(Image& img, StretchParams& sp);