#include "htn_helper.hh"

namespace Htn {

int Initialize(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    // parse parameters into FLAGS_<DECLARE_xxx>
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return 0;
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

}