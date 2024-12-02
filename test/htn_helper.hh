// MIT License

// Copyright (c) 2021 ByteDance Inc. All rights reserved.
// Copyright (c) 2021 Duke University.  All rights reserved.

// See LICENSE for license information

// This file defines the global information and configuration of HTN test.

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <infiniband/mlx5dv.h>
#include <infiniband/verbs.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdlib>

#include "htn_context.hh"

DECLARE_string(dev);
DECLARE_int32(gid);
DECLARE_int32(port);

DECLARE_bool(server);
DECLARE_string(connect_ip);
DECLARE_string(traffic);

DECLARE_int32(cq_sharing_num);
DECLARE_int32(mr_num_per_qp);

namespace Htn {

constexpr int kHostInfoKey = 0;
constexpr int kMemInfoKey = 1;
constexpr int kChannelInfoKey = 2;
constexpr int kGoGoKey = 3;

class connect_info {
public:
    int type;
    union {
        struct {
            union ibv_gid gid;
            int number_of_qp;
            int number_of_mem;
        } host;
        struct {
            uint64_t remote_addr;
            uint32_t remote_K;  // K short for Key because key is a sensitive word due
                                // to NDA :)
            int size;
        } memory;
        struct {
            int qp_num;  // QP number. For connection setup
            uint16_t dlid;
            uint8_t sl;
        } channel;
    } info;
};

struct test_qp {
    service_type;
    write_num;
    read_num;
    send_recv_num;
    mr_num;
    sg_num;
    data_size;
};

int Initialize(int argc, char **argv);
struct ibv_qp_attr MakeQpAttr(enum ibv_qp_state, enum ibv_qp_type,
                              int remote_qpn, const union ibv_gid &remote_gid,
                              int *attr_mask);

}