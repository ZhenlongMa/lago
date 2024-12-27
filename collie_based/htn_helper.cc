// MIT License

// Copyright (c) 2021 ByteDance Inc. All rights reserved.
// Copyright (c) 2021 Duke University.  All rights reserved.

// See LICENSE for license information#

#include "htn_helper.hh"


// FLAGS
// Communication Configuration
DEFINE_string(dev, "mlx5_0", "RDMA NIC device name");
DEFINE_int32(gid, 3, "GID of the RDMA NIC");
DEFINE_int32(port, 1234, "port number used for TCP");

DEFINE_bool(server, false, "is server");
DEFINE_string(connect_ip, "", "IP address of the server");
// DEFINE_string(traffic);
DEFINE_int32(min_rnr_timer, 14, "Minimal Receive Not Ready error");
DEFINE_int32(hop_limit, 16, "Hop limit");
DEFINE_int32(tos, 0, "Type of Service value");
DEFINE_int32(qp_timeout, 0, "QP timeout value");
DEFINE_int32(retry_cnt, 7, "QP retry count");
DEFINE_int32(rnr_retry, 7, "Receive Not Ready retry count");
DEFINE_int32(max_qp_rd_atom, 16, "max_qp_rd_atom");
DEFINE_int32(mtu, IBV_MTU_4096,
             "IBV_MTU value: 256/512/1024/2048/4096");

DEFINE_int32(send_wq_depth, 1024, "Send Work Queue depth");
DEFINE_int32(recv_wq_depth, 1024, "Recv Work Queue depth");

// DEFINE_int32(cq_sharing_num);
DEFINE_int32(mr_num_per_qp, 1, "");

// Resource Management
DEFINE_int32(cq_depth, 65536, "CQ depth");
DEFINE_int32(buf_size, 65536, "buffer size");
DEFINE_int32(buf_num, 1, "The number of buffers owned by one QP");

namespace Htn {

int Initialize(int argc, char **argv) {
    std::cout << "initialize" << std::endl;
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    // parse parameters into FLAGS_<DECLARE_xxx>
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    std::cout << "initialize finish!" << std::endl;
    return 0;
}

struct ibv_qp_init_attr MakeQpInitAttr(struct ibv_cq *send_cq,
                                       struct ibv_cq *recv_cq,
                                       int send_wq_depth, int recv_wq_depth) {
    struct ibv_qp_init_attr qp_init_attr;
    memset(&qp_init_attr, 0, sizeof(qp_init_attr));
    // qp_init_attr.qp_type = (enum ibv_qp_type)FLAGS_qp_type;
    qp_init_attr.sq_sig_all = 0;
    qp_init_attr.send_cq = send_cq;
    qp_init_attr.recv_cq = recv_cq;
    qp_init_attr.cap.max_send_wr = send_wq_depth;
    qp_init_attr.cap.max_recv_wr = recv_wq_depth;
    // qp_init_attr.cap.max_send_sge = kMaxSge;
    // qp_init_attr.cap.max_recv_sge = kMaxSge;
    // qp_init_attr.cap.max_inline_data = kMaxInline;
    return qp_init_attr;
}

struct ibv_qp_attr MakeQpAttr(enum ibv_qp_state state, enum ibv_qp_type qp_type,
                              int remote_qpn, const union ibv_gid &remote_gid,
                              int *attr_mask) {
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    *attr_mask = 0;
    switch (state) {
        case IBV_QPS_INIT:
            attr.port_num = 1;
            attr.qp_state = IBV_QPS_INIT;
            switch (qp_type) {
                case IBV_QPT_UD:
                    attr.qkey = 0;
                    *attr_mask |= IBV_QP_QKEY;
                    break;
                case IBV_QPT_UC:
                case IBV_QPT_RC:
                    attr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE |
                                            IBV_ACCESS_REMOTE_READ |
                                            IBV_ACCESS_REMOTE_ATOMIC;
                    *attr_mask |= IBV_QP_ACCESS_FLAGS;
                    break;
                default:
                    LOG(ERROR) << "Unsupported QP type: " << qp_type;
                break;
            }
            *attr_mask |= IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT;
            break;
        case IBV_QPS_RTR:
            attr.qp_state = IBV_QPS_RTR;
            *attr_mask |= IBV_QP_STATE;
            switch (qp_type) {
                case IBV_QPT_RC:
                    attr.max_dest_rd_atomic = FLAGS_max_qp_rd_atom;
                    attr.min_rnr_timer = FLAGS_min_rnr_timer;
                    *attr_mask |= IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
                case IBV_QPT_UC:
                    attr.path_mtu = (enum ibv_mtu)FLAGS_mtu;
                    attr.dest_qp_num = remote_qpn;
                    attr.rq_psn = 0;
                    attr.ah_attr.is_global = 1;
                    attr.ah_attr.grh.flow_label = 0;
                    attr.ah_attr.grh.sgid_index = FLAGS_gid;
                    attr.ah_attr.grh.hop_limit = FLAGS_hop_limit;
                    attr.ah_attr.grh.traffic_class = FLAGS_tos;
                    memcpy(&attr.ah_attr.grh.dgid, &remote_gid, 16);
                    attr.ah_attr.dlid = 0;
                    attr.ah_attr.sl = 0;
                    attr.ah_attr.src_path_bits = 0;
                    attr.ah_attr.port_num = 1;
                    *attr_mask |=
                        IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN;
                    break;
                case IBV_QPT_UD:
                    break;
            }
        break;
        case IBV_QPS_RTS:
            attr.qp_state = IBV_QPS_RTS;
            attr.sq_psn = 0;
            *attr_mask |= IBV_QP_STATE | IBV_QP_SQ_PSN;
            switch (qp_type) {
                case IBV_QPT_RC:
                    attr.timeout = FLAGS_qp_timeout;
                    attr.retry_cnt = FLAGS_retry_cnt;
                    attr.rnr_retry = FLAGS_rnr_retry;  // This is the retry counter, 7
                                                        // means that try infinitely.
                    attr.max_rd_atomic = FLAGS_max_qp_rd_atom;
                    *attr_mask |= IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY |
                                    IBV_QP_MAX_QP_RD_ATOMIC;
                case IBV_QPT_UC:
                case IBV_QPT_UD:
                    break;
            }
            break;
        default:
            break;
    }
    return attr;
}

std::vector<std::string> ParseHost(std::string host_ip) {
    std::vector<std::string> result;
    std::stringstream s_stream(host_ip);
    while (s_stream.good()) {
        std::string substr;
        getline(s_stream, substr, ',');
        result.push_back(substr);
    }
    return result;
}

// get current time in microsecond
uint64_t Now64() {
    struct timespec tv;
    int res = clock_gettime(CLOCK_REALTIME, &tv);
    return (uint64_t)tv.tv_sec * 1000000llu + (uint64_t)tv.tv_nsec / 1000;
}

// get current time in nanosecond
uint64_t Now64Ns() {
    struct timespec tv;
    int res = clock_gettime(CLOCK_REALTIME, &tv);
    return (uint64_t)tv.tv_sec * 1000000000llu + (uint64_t)tv.tv_nsec;
}

}
