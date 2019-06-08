/* Copyright 2019 Husky Data Lab, CUHK

Author: Created by Entityless (entityless@gmail.com)
*/

#pragma once

#include <mpi.h>
#include <thread>
#include <vector>
#include <atomic>
#include "proxima_weird_locker.hpp"

template<class Locker>
class Proxima {
 public:
    // TODO(Entityless): Enable dynamic allreduce scheduling
    struct AllreduceTaskInfo {
        MPI_Comm comm;
        int comm_sz, my_rank;
        bool use_tree;

        int comm_tag;
        size_t arr_length;

        double* to_merge;
        double* tmp_buffer;
    };

    // template<class DTYPE>
    void ForkThreadsForAllreduce(int nthreads, int slice);
    void FinalizeThreads();

    // template<class DTYPE>
    void CreateAllreduceTask(MPI_Comm comm, size_t arr_length, int comm_tag, double* to_merge, double* tmp_buffer);

    void AllreduceSum(double* to_merge, double* tmp_buffer, size_t arr_length, MPI_Comm comm, int tag = 0);

    void ComputeThreadSpin(int compute_tid);
    void AllreduceSumCompute(int compute_tid);
    void AllreduceSumSendrecv();

 private:
    Locker* locker_;

    AllreduceTaskInfo task_info_;
    std::atomic<bool> finalize_;

    std::vector<std::thread*> threads_;
    int nthreads_;
    int slice_;
};

#include "proxima.tpp"
