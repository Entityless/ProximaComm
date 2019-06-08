/* Copyright 2019 Husky Data Lab, CUHK

Author: Created by Entityless (entityless@gmail.com)
*/

#include "proxima_weird_locker.hpp"

using std::atomic;

ProximaWeirdLocker::ProximaWeirdLocker(int nthreads, int lead_slice) {
    nthreads_ = nthreads;
    lead_slice_ = lead_slice;
    assign_signals_ = (atomic<uint64_t>*) _mm_malloc(8 * sizeof(atomic<uint64_t>) * nthreads, 4096);
    finish_signals_ = (atomic<uint64_t>*) _mm_malloc(8 * sizeof(atomic<uint64_t>) * nthreads, 4096);

    for (int i = 0; i < nthreads * 8; i++) {
        assign_signals_[i] = finish_signals_[i] = 0;
    }
}

void ProximaWeirdLocker::Barrier(int tid) {
    if (tid == 0) {
        for (int i = 0; i < nthreads_; i++)
            assign_signals_[i * 8 + NORMAL_BARRIER] = 1;
        finish_signals_[NORMAL_BARRIER] = 1;

        while (true) {
            bool all_finished = true;
            for (int i = 1; i < nthreads_; i++) {
                if (finish_signals_[i * 8 + NORMAL_BARRIER] == 0) {
                    all_finished = false;
                    break;
                }
            }

            if (all_finished)
                break;
        }

        for (int i = 0; i < nthreads_; i++)
            finish_signals_[i * 8 + NORMAL_BARRIER] = 0;
    } else {
        while (true)
            if (assign_signals_[tid * 8 + NORMAL_BARRIER] == 1)
                break;

        assign_signals_[tid * 8 + NORMAL_BARRIER] = 0;
        finish_signals_[tid * 8 + NORMAL_BARRIER] = 1;
    }
}

void ProximaWeirdLocker::AsyncLeadBarrier(int tid) {
    if (tid == 0) {
        // check workers
        uint64_t my_step = assign_signals_[ASYNC_LEAD_BARRIER];

        while (true) {
            bool all_finished = true;
            for (int i = 1; i < nthreads_; i++) {
                if (assign_signals_[i * 8 + ASYNC_LEAD_BARRIER] + lead_slice_ - 1 < my_step) {
                    all_finished = false;
                    break;
                }
            }

            if (all_finished)
                break;
        }

        assign_signals_[ASYNC_LEAD_BARRIER]++;
    } else {
        uint64_t my_step = assign_signals_[ASYNC_LEAD_BARRIER + tid * 8];

        while (true)
            if (assign_signals_[ASYNC_LEAD_BARRIER] > my_step)
                break;

        assign_signals_[ASYNC_LEAD_BARRIER + tid * 8]++;
    }
}
