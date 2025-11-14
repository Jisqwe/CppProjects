#include "ContrastFix.h"
#include <iostream>

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Args args;
    if (!parse_args(argc, argv, args)){
        return 1;
    }

    if (args.is_omp) {
        if (!args.threads_default) {
            omp_set_num_threads(args.threads);
        }

        bool cli_sets_kind = !args.schedule_kind.empty();
        bool cli_sets_chunk = (args.chunk_size > 0);
        char* env_sched = std::getenv("OMP_SCHEDULE");

        if (cli_sets_kind || cli_sets_chunk) {
            std::string kind_str = cli_sets_kind ? args.schedule_kind : std::string("static");
            int chunk = cli_sets_chunk ? args.chunk_size   : SCHEDULE_CHUNK_SIZE;
            omp_sched_t kind = (kind_str == "dynamic") ? omp_sched_dynamic : omp_sched_static;
            omp_set_schedule(kind, chunk);
        } else if (!env_sched || *env_sched == '\0') {
            omp_set_schedule(omp_sched_static, SCHEDULE_CHUNK_SIZE);
        }
    }

    Image img;
    if (!read_pnm(args.in_path, img)){
        return 1;
    }

    double t0 = omp_get_wtime();

    int used_threads = 1;
    if (args.is_omp) {
        #pragma omp parallel
        {
            #pragma omp single
            {
                used_threads = omp_get_num_threads();
            }
        }
        StretchParams sp = calc_params_omp(img, args.coef);
        stretch_omp(img, sp);
    } else {
        used_threads = 1;
        StretchParams sp = calc_params_no_omp(img, args.coef);
        stretch_no_omp(img, sp);
    }

    double t1 = omp_get_wtime();
    double ms = (t1 - t0) * 1000.0;

    if(!write_pnm(args.out_path, img)){
        return 1;
    }
    std::printf("Time (%i threads): %lg\n", used_threads, ms);
    return 0;
}