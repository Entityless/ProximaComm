/* Copyright 2019 Husky Data Lab, CUHK

Author: Created by Entityless (entityless@gmail.com)
*/

#ifndef BLOCK_SIZE
#define BLOCK_SIZE(block_id, total_blocks, n) ((n / total_blocks) + ((n % total_blocks > block_id) ? 1 : 0))
#endif

#ifndef BLOCK_LOW
#define BLOCK_LOW(block_id, total_blocks, n) ((n / total_blocks) * block_id + ((n % total_blocks > block_id) ? block_id : n % total_blocks))
#endif

template<class Locker>
void Proxima<Locker>::CreateAllreduceTask(MPI_Comm comm, size_t arr_length, int comm_tag, double* to_merge, double* tmp_buffer) {
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

    task_info_.comm_tag = comm_tag;
    task_info_.arr_length = arr_length;
    task_info_.to_merge = to_merge;
    task_info_.tmp_buffer = tmp_buffer;
}

template<class Locker>
void Proxima<Locker>::ForkThreadsForAllreduce(int nthreads, int slice) {
    finalize_ = false;
    locker_ = new Locker(nthreads + 1, slice);
    for (int i = 0; i < nthreads; i++) {
        threads_.emplace_back(new std::thread(&Proxima<Locker>::ComputeThreadSpin, this, i));
    }

    nthreads_ = nthreads;
    slice_ = slice;
}

template<class Locker>
void Proxima<Locker>::FinalizeThreads() {
    finalize_ = true;
    locker_->Barrier(0);
    for (auto* compute_thread : threads_)
        compute_thread->join();
}

template<class Locker>
void Proxima<Locker>::ComputeThreadSpin(int compute_tid) {
    while (true) {
        locker_->Barrier(compute_tid + 1);
        bool finalize = finalize_;
        if (finalize)
            break;
        AllreduceSumCompute(compute_tid);
    }
}

template<class Locker>
void Proxima<Locker>::AllreduceSum(double* to_merge, double* tmp_buffer, size_t arr_length, MPI_Comm comm, int tag) {
    CreateAllreduceTask(comm, arr_length, tag, to_merge, tmp_buffer);

    if (!task_info_.use_tree) {
        MPI_Allreduce(MPI_IN_PLACE, to_merge, arr_length, MPI_DOUBLE, MPI_SUM, comm);
        return;
    }

    locker_->Barrier(0);
    AllreduceSumSendrecv();
}

template<class Locker>
void Proxima<Locker>::AllreduceSumSendrecv() {
    int bitmask = 1;
    while (bitmask < task_info_.comm_sz) {
        int partner = task_info_.my_rank ^ bitmask;

        for (int i = 0; i < slice_; i++) {
            int slice_start = BLOCK_LOW(i, slice_, task_info_.arr_length);
            int slice_len = BLOCK_SIZE(i, slice_, task_info_.arr_length);
            MPI_Sendrecv(task_info_.to_merge + slice_start, slice_len, MPI_DOUBLE, partner, task_info_.comm_tag,
                         task_info_.tmp_buffer + slice_start, slice_len, MPI_DOUBLE, partner, task_info_.comm_tag, 
                         task_info_.comm, MPI_STATUS_IGNORE);
            // locker_->Barrier(0);
            locker_->AsyncLeadBarrier(0);
        }


        bitmask <<= 1;
    }

    locker_->Barrier(0);
}

template<class Locker>
void Proxima<Locker>::AllreduceSumCompute(int compute_tid) {
    int bitmask = 1;

    while (bitmask < task_info_.comm_sz) {
        int partner = task_info_.my_rank ^ bitmask;

        for (int i = 0; i < slice_; i++) {
            // locker_->Barrier(compute_tid + 1);
            locker_->AsyncLeadBarrier(compute_tid + 1);

            int slice_start = BLOCK_LOW(i, slice_, task_info_.arr_length);
            int slice_len = BLOCK_SIZE(i, slice_, task_info_.arr_length);

            int my_start = BLOCK_LOW(compute_tid, nthreads_, slice_len) + slice_start;
            int my_length = BLOCK_SIZE(compute_tid, nthreads_, slice_len);

            for (int i = 0; i < my_length; i++)
                task_info_.to_merge[my_start + i] += task_info_.tmp_buffer[my_start + i];
        }

        bitmask <<= 1;
    }

    locker_->Barrier(compute_tid + 1);
}
