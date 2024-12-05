#ifndef HTN_CONTEXT_HH
#define HTN_CONTEXT_HH

#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <fstream>

#include "htn_helper.hh"
#include "htn_endpoint.hh"

namespace Htn {

union collie_cq {
    struct ibv_cq *cq;
    struct ibv_cq_ex *cq_ex;
};

class htn_context {
public:
    std::string device_name_;
    union ibv_gid local_gid_;
    std::string local_ip_;
    struct ibv_context *ctx_;
    uint16_t lid_;
    uint8_t sl_;
    int port_;
    std::queue<int> ids_;

    // store all test case, each unit is a test metadata for one QP
    std::vector<test_qp> test_case;
    
    // store all endpoints(QPs)
    std::vector<htn_endpoint *> endpoints_;
    std::vector<struct ibv_pd *> pds_;

    // Transportation
    std::vector<union collie_cq> send_cqs_;
    std::vector<union collie_cq> recv_cqs_;

    int total_mr_num_ = 0;

    // Connection Setup: Server side
    int Listen();
    int ServerDatapath();

    // std::vector<std::vector<htn_region *>> local_mempool_ =
    //     std::vector<std::vector<htn_region *>>(2);
    std::vector<htn_region *> send_mempool_;
    std::vector<htn_region *> recv_mempool_;

    // Connection Setup: Client side
    int Connect(const char *server, int port, int connid);
    int ClientDatapath();

    int Init();
    int InitDevice();
    int InitMemory();
    int InitIds();
    int InitTransport();
    int AcceptHandler(int connfd);

    void SetInfoByBuffer(struct connect_info *info, htn_buffer *buf);

    uint32_t current_buf_id_ = 0;

    int num_of_hosts_ = 0;  // How many hosts to set up connections
    int num_qp_per_host_ = 0;  // How many connections each host will set

    // Assitant function: Randomly choose a buffer
    // 0 indicates send buffer
    // 1 indicates recv buffer
    htn_buffer *PickNextBuffer(int idx) {
        if (idx != 0 && idx != 1) {
            return nullptr;
        } 
        // if (local_mempool_[idx].empty()) {
        //     return nullptr;
        // }
        htn_buffer* buf = local_mempool_[idx][current_buf_id_]->GetBuffer();
        current_buf_id_++;
        if (current_buf_id_ == local_mempool_[idx].size()) {
            current_buf_id_ = 0;
        } 
        return buf;
    }
};

}
#endif