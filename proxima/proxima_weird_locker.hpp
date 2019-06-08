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
    ProximaWeirdLocker(int nthreads);

    void Barrier(int tid);

 private:
    std::atomic<uint64_t>* assign_signals_;
    std::atomic<uint64_t>* finish_signals_;
    int nthreads_;
};
