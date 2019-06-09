/* Copyright 2019 Husky Data Lab, CUHK

Author: Created by Entityless (entityless@gmail.com)
*/

#pragma once

#include <assert.h>
#include <mpi.h>
#include <thread>
#include <vector>
#include <atomic>

template<class Locker>
class ProximaComm {
 public:
    struct AllreduceTaskInfo {
        MPI_Comm comm;
        int comm_sz, my_rank;
        bool use_tree;  // must be true

        int comm_tag;
        size_t arr_element_count;
        size_t element_size;

        void* to_merge;
        void* tmp_buffer;
    };

    void ForkThreadsForAllreduce(int nthreads, int slice);
    void FinalizeThreads();

    // template<class DTYPE>
    void CreateAllreduceTask(MPI_Comm comm, size_t arr_element_count, int comm_tag, void* to_merge, void* tmp_buffer);

    void AllreduceSum(void* to_merge, void* tmp_buffer, size_t arr_element_count, MPI_Comm comm, int tag = 0);

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

#include "proxima_comm.tpp"
