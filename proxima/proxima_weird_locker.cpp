/* Copyright 2019 Husky Data Lab, CUHK

Author: Created by Entityless (entityless@gmail.com)
*/

#include "proxima_weird_locker.hpp"

using std::atomic;

ProximaWeirdLocker::ProximaWeirdLocker(int nthreads) {
    nthreads_ = nthreads;
    assign_signals_ = (atomic<uint64_t>*) _mm_malloc(8 * sizeof(atomic<uint64_t>) * nthreads, 4096);
    finish_signals_ = (atomic<uint64_t>*) _mm_malloc(8 * sizeof(atomic<uint64_t>) * nthreads, 4096);

    for (int i = 0; i < nthreads; i++) {
        assign_signals_[i * 8] = finish_signals_[i * 8] = 0;
    }
}

void ProximaWeirdLocker::Barrier(int tid) {
    if (tid == 0) {
        for (int i = 0; i < nthreads_; i++)
            assign_signals_[i * 8] = 1;
        finish_signals_[0] = 1;

        while (true) {
            bool all_finished = true;
            for (int i = 0; i < nthreads_; i++) {
                if (finish_signals_[i * 8] == 0) {
                    all_finished = false;
                    break;
                }
            }

            if (all_finished)
                break;
        }

        for (int i = 0; i < nthreads_; i++)
            finish_signals_[i * 8] = 0;
    } else {
        while (true)
            if (assign_signals_[tid * 8] == 1)
                break;

        assign_signals_[tid * 8] = 0;
        finish_signals_[tid * 8] = 1;
    }
}
