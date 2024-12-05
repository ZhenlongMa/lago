// MIT License

// Copyright (c) 2021 ByteDance Inc. All rights reserved.
// Copyright (c) 2021 Duke University.  All rights reserved.

// See LICENSE for license information

#ifndef HTN_MEMORY_HH
#define HTN_MEMORY_HH

#include "htn_helper.hh"

namespace Htn{

// test data memory pool
class htn_buffer {
    uint64_t addr_;
    uint32_t size_;
    uint32_t local_key_;
    uint32_t remote_key_;
    htn_buffer(uint64_t addr, uint32_t size, uint32_t local_key, uint32_t remote_key) :
        addr_(addr), size_(size), local_key_(local_key), remote_key_(remote_key) {}
};

// An HTN region contains multiple HTN buffers
class htn_region {
public:
    struct ibv_mr *mr_ = nullptr;
    struct ibv_pd *pd_ = nullptr;
    int numa_ = -1;
    int num_ = 0; // number of HTN buffers
    uint32_t size_ = 0;
    bool align_ = false;
    std::queue<htn_buffer *> buffers_;

    htn_region(struct ibv_pd *pd, size_t size, int n, bool align, int numa)
        : pd_(pd), numa_(numa), num_(n), size_(size), align_(align) {}

    // Allocate from main memory
    int Mallocate();
    // Pick a buffer in the order of FIFO
    htn_buffer *GetBuffer();
};

}

#endif