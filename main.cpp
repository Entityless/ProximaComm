/* Copyright 2019 Husky Data Lab, CUHK

Author: Created by Entityless (entityless@gmail.com)
*/

#include <mpi.h>
#include <iostream>
#include <omp.h>
#include "proxima/proxima_weird_locker.hpp"
#include "proxima/proxima_comm.hpp"
#include <numeric>

using namespace std;

template <class DTYPE, MPI_Datatype mpi_dtype>
void Benchmark(int profile_cnt, int loop_cnt, int arr_sz, int comp_nthreads, int slice) {
    int comm_sz;
    int my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (my_rank == 0) {
        printf("profile_cnt = %d, loop_cnt = %d, arr_sz = %d, comp_nthreads = %d, slice = %d\n",
                profile_cnt, loop_cnt, arr_sz, comp_nthreads, slice);
        printf("sizeof(DTYPE) = %d\n", sizeof(DTYPE));
        fflush(stdout);
    }

    DTYPE* send_arr = (DTYPE*) _mm_malloc(sizeof(DTYPE) * arr_sz, 4096);
    DTYPE* recv_arr = (DTYPE*) _mm_malloc(sizeof(DTYPE) * arr_sz, 4096);
    DTYPE sum, *all_sum = new DTYPE[comm_sz];
    double stt, edt;
    string sum_str;

    auto str_append_double = [](std::string a, DTYPE b) { return std::move(a) + "{" + std::to_string(b) + "}";};

    for (int l = 0; l < profile_cnt; l++) {
        for (int i = 0; i < arr_sz; i++) {
            if (my_rank == 0)
                send_arr[i] = comm_sz - 1.000001;
            else
                send_arr[i] = -1.0;
        }


        stt = MPI_Wtime();
        for (int i = 0; i < loop_cnt; i++) {
            MPI_Allreduce(MPI_IN_PLACE, send_arr, arr_sz, mpi_dtype, MPI_SUM, MPI_COMM_WORLD);
        }
        edt = MPI_Wtime();

        sum = 0;
        for (int i = 0; i < arr_sz; i++)
            sum += send_arr[i];
        MPI_Allgather(&sum, 1, mpi_dtype, all_sum, 1, mpi_dtype, MPI_COMM_WORLD);
        sum_str = accumulate(all_sum, all_sum + comm_sz, string(""), str_append_double);

        if (my_rank == 0)
            printf("### Normal allreduce finished, checksum = %s, duration = %f\n", sum_str.c_str(), edt - stt);
        fflush(stdout);
        MPI_Barrier(MPI_COMM_WORLD);

        for (int i = 0; i < arr_sz; i++) {
            if (my_rank == 0)
                send_arr[i] = comm_sz - 1.000001;
            else
                send_arr[i] = -1.0;
        }

        ProximaComm<ProximaWeirdLocker> p;
        p.ForkThreadsForAllreduce(comp_nthreads, slice);

        stt = MPI_Wtime();
        for (int i = 0; i < loop_cnt; i++) {
            p.AllreduceSum<DTYPE>(send_arr, recv_arr, arr_sz, MPI_COMM_WORLD);
        }
        edt = MPI_Wtime();
        p.FinalizeThreads();  // do not influence normal allreduce

        sum = 0;
        for (int i = 0; i < arr_sz; i++)
            sum += send_arr[i];
        MPI_Allgather(&sum, 1, mpi_dtype, all_sum, 1, mpi_dtype, MPI_COMM_WORLD);
        sum_str = accumulate(all_sum, all_sum + comm_sz, string(""), str_append_double);

        if (my_rank == 0)
            printf("*** Proxima allreduce finished, checksum = %s, duration = %f\n", sum_str.c_str(), edt - stt);
        fflush(stdout);
        MPI_Barrier(MPI_COMM_WORLD);
    }

    _mm_free(send_arr);
    _mm_free(recv_arr);
    delete[] all_sum;
}


// mpirun -n <nprocs> -f <machine_file> ./test_proxima 4 10 10000000 1 200
int main(int argc, char** argv) {
    if (argc != 6) {
        printf("params: <profile count> <loop count> <arr size> <sum nthreads> <slice>\n");
        exit(0);
    }

    int profile_cnt, loop_cnt, arr_sz, comp_nthreads, slice;
    profile_cnt = atoi(argv[1]);
    loop_cnt = atoi(argv[2]);
    arr_sz = atoi(argv[3]);
    comp_nthreads = atoi(argv[4]);
    slice = atoi(argv[5]);

    MPI_Init(nullptr, nullptr);

    Benchmark<double, MPI_DOUBLE>(profile_cnt, loop_cnt, arr_sz, comp_nthreads, slice);
    Benchmark<float, MPI_FLOAT>(profile_cnt, loop_cnt, arr_sz, comp_nthreads, slice);

    MPI_Finalize();

    return 0;
}

