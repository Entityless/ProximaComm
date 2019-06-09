/* Copyright 2019 Husky Data Lab, CUHK

Author: Created by Entityless (entityless@gmail.com)
*/

#ifndef BLOCK_SIZE
#define BLOCK_SIZE(block_id, total_blocks, n) ((n / total_blocks) + ((n % total_blocks > block_id) ? 1 : 0))
#endif

#ifndef BLOCK_LOW
#define BLOCK_LOW(block_id, total_blocks, n) ((n / total_blocks) * block_id + ((n % total_blocks > block_id) ? block_id : n % total_blocks))
#endif

template<class Locker> template<class DTYPE>
void ProximaComm<Locker>::CreateAllreduceTask(MPI_Comm comm, size_t arr_element_count, int comm_tag, void* to_merge, void* tmp_buffer) {
    task_info_.comm = comm;
    MPI_Comm_size(MPI_COMM_WORLD, &task_info_.comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &task_info_.my_rank);

    int max_2_pow = 1;
    while (max_2_pow < task_info_.comm_sz)
        max_2_pow *= 2;

    if (max_2_pow == task_info_.comm_sz && max_2_pow != 1) {
        task_info_.use_tree = true;
    } else {
        task_info_.use_tree = false;
    }

    assert(task_info_.use_tree);

    task_info_.comm_tag = comm_tag;
    task_info_.arr_element_count = arr_element_count;
    task_info_.element_size = sizeof(DTYPE);
    task_info_.to_merge = to_merge;
    task_info_.tmp_buffer = tmp_buffer;

    task_info_.compute_func_ptr = &ProximaComm<Locker>::AllreduceSumCompute<DTYPE>;
}

template<class Locker>
void ProximaComm<Locker>::ForkThreadsForAllreduce(int nthreads, int slice) {
    finalize_ = false;
    locker_ = new Locker(nthreads + 1, slice);
    for (int i = 0; i < nthreads; i++) {
        threads_.emplace_back(new std::thread(&ProximaComm<Locker>::ComputeThreadSpin, this, i));
    }

    nthreads_ = nthreads;
    slice_ = slice;
}

template<class Locker>
void ProximaComm<Locker>::FinalizeThreads() {
    finalize_ = true;
    locker_->Barrier(0);
    for (auto* compute_thread : threads_)
        compute_thread->join();
}

template<class Locker>
void ProximaComm<Locker>::ComputeThreadSpin(int compute_tid) {
    while (true) {
        locker_->Barrier(compute_tid + 1);
        bool finalize = finalize_;
        if (finalize)
            break;
        AllreduceSumComputeOuter(compute_tid);
    }
}

template<class Locker> template<class DTYPE>
void ProximaComm<Locker>::AllreduceSum(void* to_merge, void* tmp_buffer, size_t arr_element_count, MPI_Comm comm, int tag) {
    CreateAllreduceTask<DTYPE>(comm, arr_element_count, tag, to_merge, tmp_buffer);

    locker_->Barrier(0);
    AllreduceSumSendrecv<DTYPE>();
}

template<class Locker> template<class DTYPE>
void ProximaComm<Locker>::AllreduceSumSendrecv() {
    int bitmask = 1;
    while (bitmask < task_info_.comm_sz) {
        int partner = task_info_.my_rank ^ bitmask;

        for (int i = 0; i < slice_; i++) {
            int slice_start = BLOCK_LOW(i, slice_, task_info_.arr_element_count);
            int slice_len = BLOCK_SIZE(i, slice_, task_info_.arr_element_count);
            MPI_Sendrecv(task_info_.to_merge + slice_start * sizeof(DTYPE), slice_len * sizeof(DTYPE), MPI_BYTE, partner, task_info_.comm_tag,
                         task_info_.tmp_buffer + slice_start * sizeof(DTYPE), slice_len * sizeof(DTYPE), MPI_BYTE, partner, task_info_.comm_tag, 
                         task_info_.comm, MPI_STATUS_IGNORE);
            locker_->AsyncLeadBarrier(0);
        }

        bitmask <<= 1;
    }

    locker_->Barrier(0);
}


template<class Locker>
void ProximaComm<Locker>::AllreduceSumComputeOuter(int compute_tid) {
    void (ProximaComm<Locker>::*compute_func_ptr)(int) = task_info_.compute_func_ptr;
    (this->*compute_func_ptr)(compute_tid);
}

template<class Locker> template<class DTYPE>
void ProximaComm<Locker>::AllreduceSumCompute(int compute_tid) {
    int bitmask = 1;

    DTYPE* to_merge = (DTYPE*) task_info_.to_merge;
    DTYPE* tmp_buffer = (DTYPE*) task_info_.tmp_buffer;

    while (bitmask < task_info_.comm_sz) {
        int partner = task_info_.my_rank ^ bitmask;

        for (int i = 0; i < slice_; i++) {
            locker_->AsyncLeadBarrier(compute_tid + 1);

            int slice_start = BLOCK_LOW(i, slice_, task_info_.arr_element_count);
            int slice_len = BLOCK_SIZE(i, slice_, task_info_.arr_element_count);

            int my_start = BLOCK_LOW(compute_tid, nthreads_, slice_len) + slice_start;
            int my_length = BLOCK_SIZE(compute_tid, nthreads_, slice_len);

            for (int i = 0; i < my_length; i++)
                to_merge[my_start + i] += tmp_buffer[my_start + i];
        }

        bitmask <<= 1;
    }

    locker_->Barrier(compute_tid + 1);
}
