/* Copyright 2019 Husky Data Lab, CUHK

Author: Created by Entityless (entityless@gmail.com)
*/

#pragma once

#include <stdint.h>

#if defined(__INTEL_COMPILER)
#include <malloc.h>
#else
#include <mm_malloc.h>
#endif  // defined(__GNUC__)

#include <atomic>

class ProximaWeirdLocker {
 public:
    ProximaWeirdLocker(int nthreads, int lead_slice);

    void Barrier(int tid);
    void AsyncLeadBarrier(int tid);

 private:
    enum FlagIdentifer {
        NORMAL_BARRIER = 0,
        ASYNC_LEAD_BARRIER = 1,
        MAX_LIMIT = 8
    };

    std::atomic<uint64_t>* assign_signals_;
    std::atomic<uint64_t>* finish_signals_;
    int nthreads_, lead_slice_;
};
