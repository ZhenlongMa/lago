// MIT License

// Copyright (c) 2021 ByteDance Inc. All rights reserved.
// Copyright (c) 2021 Duke University.  All rights reserved.

// See LICENSE for license information

#include "htn_memory.hh"

#include <malloc.h>

namespace Htn {

int htn_region::Mallocate() {
    auto buf_size = num_ * size_;
    int mrflags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
                    IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC;
    char *buffer = nullptr;

    if (align_) {
        buffer = (char *)memalign(sysconf(_SC_PAGESIZE), buf_size);
    } else {
        buffer = (char *)malloc(buf_size);
    }

    if (!buffer) {
        PLOG(ERROR) << "Memory Allocation Failed";
        return -1;
    }
    mr_ = ibv_reg_mr(pd_, buffer, buf_size, mrflags);
    if (!mr_) {
        PLOG(ERROR) << "ibv_reg_mr() failed";
        return -1;
    }
    for (size_t i = 0; i < num_; i++) {
        htn_buffer *rbuf = new htn_buffer((uint64_t)(buffer + size_ * i), size_,
                                            mr_->lkey, mr_->rkey);
        buffers_.push(rbuf);
    }
    return 0;
}

htn_buffer *htn_region::GetBuffer() {
    if (buffers_.empty()) {
        LOG(ERROR) << "The MR's buffer is empty";
        return nullptr;
    }
    auto rbuf = buffers_.front();
    buffers_.pop();
    buffers_.push(rbuf);
    return rbuf;
}

}