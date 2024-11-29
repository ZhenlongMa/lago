#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "htn_helper.hh"

namespace Htn {

union collie_cq {
    struct ibv_cq *cq;
    struct ibv_cq_ex *cq_ex;
};

class htn_context {

    string device_name_;
    union ibv_gid local_gid_;
    string local_ip_;
    struct ibv_context *ctx_;
    uint16_t lid_;
    uint8_t sl_;
    int port_;
    std::queue<int> ids_;
    
    // store all endpoints(QPs)
    std::vector<rdma_endpoint *> endpoints_;
    std::vector<struct ibv_pd *> pds_;

    // Transportation
    std::vector<union collie_cq> send_cqs_;
    std::vector<union collie_cq> recv_cqs_;

    // Connection Setup: Server side
    int Listen();
    int ServerDatapath();

    // Connection Setup: Client side
    int Connect(const char *server, int port, int connid);
    int ClientDatapath();

    int Init();

};

}