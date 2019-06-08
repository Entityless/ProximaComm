/* Copyright 2019 Husky Data Lab, CUHK

Author: Created by Entityless (entityless@gmail.com)
*/

#include <mpi.h>
#include <iostream>
#include <omp.h>
#include "proxima/proxima_weird_locker.hpp"
#include "proxima/proxima.hpp"

using namespace std;

// loop_cnt arr_sz comp_nthreads
int main(int argc, char** argv) {
    if (argc != 5) {
        printf("params: <loop count> <arr size> <sum nthreads> <slice>\n");
        exit(0);
    }

    int loop_cnt, arr_sz, comp_nthreads, slice;
    loop_cnt = atoi(argv[1]);
    arr_sz = atoi(argv[2]);
    comp_nthreads = atoi(argv[3]);
    slice = atoi(argv[4]);

    MPI_Init(nullptr, nullptr);

    int comm_sz;
    int my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (my_rank == 0) {
        printf("loop_cnt = %d, arr_sz = %d, comp_nthreads = %d\n", loop_cnt, arr_sz, comp_nthreads);
        fflush(stdout);
    }

    double* send_arr = (double*) _mm_malloc(sizeof(double) * arr_sz, 4096);
    double* recv_arr = (double*) _mm_malloc(sizeof(double) * arr_sz, 4096);
    double sum;

    for (int i = 0; i < arr_sz; i++) {
        if (my_rank == 0)
            send_arr[i] = comm_sz - 1.000001;
        else
            send_arr[i] = -1.0;
    }

    double stt, edt;

    stt = MPI_Wtime();
    for (int i = 0; i < loop_cnt; i++) {
        MPI_Allreduce(MPI_IN_PLACE, send_arr, arr_sz, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    }
    edt = MPI_Wtime();

    sum = 0;
    for (int i = 0; i < arr_sz; i++)
        sum += send_arr[i];

    MPI_Barrier(MPI_COMM_WORLD);
    printf("normal allreduce finished, checksum = %f, duration = %f\n", sum, edt - stt);
    fflush(stdout);
    MPI_Barrier(MPI_COMM_WORLD);

    for (int i = 0; i < arr_sz; i++) {
        if (my_rank == 0)
            send_arr[i] = comm_sz - 1.000001;
        else
            send_arr[i] = -1.0;
    }

    Proxima<ProximaWeirdLocker> p;
    p.ForkThreadsForAllreduce(comp_nthreads, slice);

    stt = MPI_Wtime();
    for (int i = 0; i < loop_cnt; i++) {
        p.AllreduceSum(send_arr, recv_arr, arr_sz, MPI_COMM_WORLD);
    }
    edt = MPI_Wtime();

    sum = 0;
    for (int i = 0; i < arr_sz; i++)
        sum += send_arr[i];

    p.FinalizeThreads();

    MPI_Barrier(MPI_COMM_WORLD);
    printf("weird allreduce finished, checksum = %f, duration = %f\n", sum, edt - stt);
    fflush(stdout);
    MPI_Barrier(MPI_COMM_WORLD);

    // p.Allreduce();

    MPI_Finalize();

    return 0;
}

