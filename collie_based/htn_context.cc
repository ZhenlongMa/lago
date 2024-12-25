// MIT License

// Copyright (c) 2021 ByteDance Inc. All rights reserved.
// Copyright (c) 2021 Duke University.  All rights reserved.

// See LICENSE for license information

#include "htn_context.hh"

namespace Htn {

int htn_context::Init() {
    // file format: 
    // service_type write_num read_num send_recv_num mr_num sg_num data_size
    std::ifstream test_file("test_case_demo");
    std::string qp_info;
    while (std::getline(test_file, qp_info)) {
        test_qp test;
        std::stringstream qp_info_stream(qp_info);
        qp_info_stream >> test.service_type;
        qp_info_stream >> test.write_num;
        qp_info_stream >> test.read_num;
        qp_info_stream >> test.send_recv_num;
        qp_info_stream >> test.mr_num;
        total_mr_num_ += test.mr_num;
        qp_info_stream >> test.sg_num;
        qp_info_stream >> test.data_size;
        test_case.push_back(test);
    }
    num_qp_per_host_ = test_case.size();
    if (InitDevice() < 0) {
        LOG(ERROR) << "InitDevice() failed";
        return -1;
    }
    if (InitMemory() < 0) {
        LOG(ERROR) << "InitMemory() failed";
        return -1;
    }
    if (InitTransport() < 0) {
        LOG(ERROR) << "InitTransport() failed";
        return -1;
    }
    return 0;
}

int htn_context:: InitDevice() {
    struct ibv_device *dev = nullptr;
    struct ibv_device **device_list = nullptr;
    int dev_num;
    bool found_device = false;
    device_list = ibv_get_device_list(&dev_num);
    if (!device_list) {
        LOG(ERROR) << "ibv_get_device_list() failed!";
        return -1;
    }
    for (int i = 0; i < dev_num; i++) {
        dev = device_list[i];
        if (!strncmp(ibv_get_device_name(dev), device_name_.c_str(), strlen(device_name_.c_str()))) {
            found_device = true;
            break;
        }
    }
    if (!found_device) {
        LOG(ERROR) << "Device" << device_name_ << "not found!";
    }
    ctx_ = ibv_open_device(dev);
    if (!ctx_) {
        LOG(ERROR) << "cannot open device";
        return -1;
    }
    int num_of_qps = InitIds();
    endpoints_.resize(num_of_qps, nullptr);
    struct ibv_port_attr port_attr;
    if (ibv_query_port(ctx_, 1, &port_attr)) {
        PLOG(ERROR) << "ibv_query_port() failed";
        exit(1);
    }
    lid_ = port_attr.lid;
    // sl_ = port_attr.sm_sl;
    port_ = FLAGS_port;
    return 0;
}

int htn_context::InitIds() {
    while (!ids_.empty()) {
        ids_.pop();
    }
    auto num_of_qps = num_of_hosts_ * num_qp_per_host_;
    for (int i = 0; i < num_of_qps; i++) {
        ids_.push(i);
    }
    return num_of_qps;
}

int htn_context::InitMemory() {
    // In default, each MR has a identical PD.
    int pd_num = 1;
    for (int i = 0; i < pd_num; i++) {
        struct ibv_pd *pd = ibv_alloc_pd(ctx_);
        if (!pd) {
            PLOG(ERROR) << "ibv_alloc_pd() failed";
            return -1;
        }
        pds_.push_back(pd);
    }
    int buffer_size = FLAGS_buf_size;
    for (int i = 0; i < total_mr_num_; i++) {
        htn_region* region = new htn_region(pds_[i], buffer_size, FLAGS_buf_num, false, 0);
        if (region->Mallocate()) {
            LOG(ERROR) << "Region Memory allocation failed";
            break;
        }
        send_mempool_.push_back(region);
        region = new htn_region(pds_[i], buffer_size, FLAGS_buf_num, false, 0);
        if (region->Mallocate()) {
            LOG(ERROR) << "Region Memory allocation failed";
            break;
        }
        recv_mempool_.push_back(region);
    }

    // Allocate CQ
    int cqn = num_of_hosts_ * num_qp_per_host_;
    for (int i = 0; i < cqn; i++) {
        union htn_cq send_cq;
        union htn_cq recv_cq;
        send_cq.cq =
            ibv_create_cq(ctx_, FLAGS_cq_depth / cqn, nullptr, nullptr, 0);
        if (!send_cq.cq) {
            PLOG(ERROR) << "ibv_create_cq() failed";
            return -1;
        }
        recv_cq.cq =
            ibv_create_cq(ctx_, FLAGS_cq_depth / cqn, nullptr, nullptr, 0);
        if (!recv_cq.cq) {
            PLOG(ERROR) << "ibv_create_cq() failed";
            return -1;
        }
        send_cqs_.push_back(send_cq);
        recv_cqs_.push_back(recv_cq);
    }
    return 0;
}

int htn_context::InitTransport() {
    htn_endpoint *ep = nullptr;
    int cnt = 0;
    while (!ids_.empty()) {
        int id = ids_.front();
        ids_.pop();
        if (endpoints_[id]) {
            delete endpoints_[id];
        }
        struct ibv_qp_init_attr qp_init_attr = MakeQpInitAttr(
            GetSendCq(id), GetRecvCq(id), FLAGS_send_wq_depth, FLAGS_recv_wq_depth);
        ibv_qp *qp = ibv_create_qp(pds_[id], &qp_init_attr);
        if (!qp) {
            PLOG(ERROR) << "ibv_create_qp() failed";
            delete ep;
            return -1;
        }
        ep = new htn_endpoint(id, qp);
        // ep->SetMaster(this);
        endpoints_[id] = ep;
    }
    return 0;
}

int htn_context::Listen() {
    struct addrinfo *res, *t;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char *service;
    int sockfd = -1, err, n;
    int *connfd;
    if (asprintf(&service, "%d", port_) < 0) return -1; // set service the port
    if (getaddrinfo(nullptr, service, &hints, &res)) { // get a linked list of address structures
                                                       // for a specified port and hint
        LOG(ERROR) << gai_strerror(n) << " for port " << port_;
        free(service);
        return -1;
    }
    for (t = res; t; t = t->ai_next) {
        sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
        if (sockfd >= 0) {
            n = 1;
            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n));
            if (!bind(sockfd, t->ai_addr, t->ai_addrlen)) break; // if bind successfully, break
            close(sockfd);
            sockfd = -1;
        }
    }
    freeaddrinfo(res);
    free(service);
    if (sockfd < 0) {
        LOG(ERROR) << "Couldn't listen to port " << port_;
        return -1;
    }
    LOG(INFO) << "About to listen on port " << port_;
    err = listen(sockfd, 1024);
    if (err) {
        PLOG(ERROR) << "listen() failed";
        return -1;
    }
    LOG(INFO) << "Server listen thread starts";
    while (true) {
        connfd = (int *)malloc(sizeof(int));
        *connfd = accept(sockfd, nullptr, 0);
        if (*connfd < 0) {
            PLOG(ERROR) << "Accept Error";
            break;
        }
        // [TODO] connection handler
        std::thread handler =
            std::thread(&htn_context::AcceptHandler, this, *connfd);
        handler.detach();
        free(connfd);
    }
    // The loop shall never end.
    free(connfd);
    close(sockfd);
    return -1;
}


int htn_context::AcceptHandler(int connfd) {
    int n, number_of_qp, number_of_mem, left, right;
    char *conn_buf = (char *)malloc(sizeof(connect_info));
    connect_info *info = (connect_info *)conn_buf;
    union ibv_gid gid;
    std::vector<htn_buffer *> buffers;
    // auto reqs = ParseRecvFromStr();
    int rbuf_id = -1;
    if (!conn_buf) {
        LOG(ERROR) << "Malloc for exchange buffer failed";
        return -1;
    }
    n = read(connfd, conn_buf, sizeof(connect_info)); // read the connection information
                                                      // and store to conn_buf (info)
    if (n != sizeof(connect_info)) {
        PLOG(ERROR) << "Server Read";
        LOG(ERROR) << n << "/" << (int)sizeof(connect_info)
                << ": Couldn't read remote address";
        goto out;
    }
    if ((info->type) != kHostInfoKey) {
        LOG(ERROR) << "The first exchange type should be " << kHostInfoKey;
        goto out;
    }
    number_of_qp = (info->info.host.number_of_qp);
    number_of_mem = (info->info.host.number_of_mem);
    if (number_of_qp <= 0) {
        LOG(ERROR) << "The number of qp should be positive";
        goto out;
    }

    // numlock_.lock();
    if (num_of_recv_ + number_of_qp > num_qp_per_host_ * num_of_hosts_) { // If client's request is out of server's capacity, 
                                                                       // return ZERO back to client
        LOG(ERROR) << "QP Overflow, request rejected";
        // numlock_.unlock();
        memset(info, 0, sizeof(connect_info));
        if (write(connfd, conn_buf, sizeof(connect_info)) != sizeof(connect_info))
        PLOG(ERROR) << "Write Error";
        goto out;
    }
    left = num_of_recv_;
    num_of_recv_ += number_of_qp;
    // numlock_.unlock();
    right = left + number_of_qp;
    // Copy the remote gid.
    memcpy(&gid, &info->info.host.gid, sizeof(union ibv_gid));

    // Put local info to connect_info and send
    memset(info, 0, sizeof(connect_info));
    info->type = (kHostInfoKey);
    memcpy(&info->info.host.gid, &local_gid_, sizeof(union ibv_gid));
    info->info.host.number_of_qp = (number_of_qp);
    if (write(connfd, conn_buf, sizeof(connect_info)) != sizeof(connect_info)) {
        LOG(ERROR) << "Couldn't send local address";
        goto out;
    }

    // Get the memory info from remote
    for (int i = 0; i < number_of_mem; i++) {
        n = read(connfd, conn_buf, sizeof(connect_info));
        if (n != sizeof(connect_info)) {
            PLOG(ERROR) << "Server read";
            LOG(ERROR) << n << "/" << (int)sizeof(connect_info) << ": Read " << i
                        << " mem's info failed";
            goto out;
        }
        if ((info->type) != kMemInfoKey) {
            LOG(ERROR) << "Exchange MemInfo failed. Type received is "
                        << (info->type);
            goto out;
        }
        auto remote_buf = CreateBufferFromInfo(info);
        buffers.push_back(remote_buf);
        auto buf = PickNextBuffer(1);
        if (!buf) {
            LOG(ERROR) << "Server using buffer error";
            goto out;
        }
        SetInfoByBuffer(info, buf);
        if (write(connfd, conn_buf, sizeof(connect_info)) != sizeof(connect_info)) {
            LOG(ERROR) << "Couldn't send " << i << " memory's info";
            goto out;
        }
    }

    // rmem_lock_.lock();
    remote_mempools_.push_back(buffers);
    rbuf_id = remote_mempools_.size() - 1;
    // rmem_lock_.unlock();

    // Get the connection channel info from remote

    for (int i = left; i < right; i++) {
        auto ep = (htn_endpoint *)endpoints_[i];
        n = read(connfd, conn_buf, sizeof(connect_info));
        if (n != sizeof(connect_info)) {
            PLOG(ERROR) << "Server read";
            LOG(ERROR) << n << "/" << (int)sizeof(connect_info) << ": Read " << i
                        << " endpoint's info failed";
            goto out;
        }
        if ((info->type) != kChannelInfoKey) {
            LOG(ERROR) << "Exchange data failed. Type Error: " << (info->type);
            goto out;
        }
        SetEndpointInfo(ep, info);
        GetEndpointInfo(ep, info);
        if (write(connfd, conn_buf, sizeof(connect_info)) != sizeof(connect_info)) {
            LOG(ERROR) << "Couldn't send " << i << " endpoint's info";
            goto out;
        }
        if (ep->Activate(gid)) {
            LOG(ERROR) << "Activate Recv Endpoint " << i << " failed";
            goto out;
        }
        // Post The first batch
        int first_batch = FLAGS_recv_wq_depth;
        // int batch_size = FLAGS_recv_batch;
        int batch_size = 100; // temp set
        size_t idx = 0;
        while (ep->recv_credits_ > 0) {
            auto num_to_post = std::min(first_batch, batch_size);
            // if (ep->PostRecv(reqs, idx, num_to_post)) {
            //     LOG(ERROR) << "The " << i << " Receiver Post first batch error";
            //     goto out;
            // }
            first_batch -= num_to_post;
        }
        ep->activated_ = true;
        ep->rmem_id_ = rbuf_id;
        ep->remote_server_ = GidToIP(gid);
        LOG(INFO) << "Endpoint " << i << " has started";
    }

    // After connection setup. Tell remote that they can send.
    n = read(connfd, conn_buf, sizeof(connect_info));
    if (n != sizeof(connect_info)) {
        PLOG(ERROR) << "Server read";
        LOG(ERROR) << n << "/" << (int)sizeof(connect_info)
                << ": Read Send request failed";
        goto out;
    }
    if ((info->type) != kGoGoKey) {
        LOG(ERROR) << "GOGO request failed";
        goto out;
    }
    memset(info, 0, sizeof(connect_info));
    info->type = (kGoGoKey);
    if (write(connfd, conn_buf, sizeof(connect_info)) != sizeof(connect_info)) {
        LOG(ERROR) << "Couldn't send GOGO!!";
        goto out;
    }
    close(connfd);
    free(conn_buf);
    return 0;
out:
    close(connfd);
    free(conn_buf);
    return -1;
}

htn_buffer *htn_context::CreateBufferFromInfo(struct connect_info *info) {
    uint64_t remote_addr = (info->info.memory.remote_addr);
    uint32_t rkey = (info->info.memory.remote_K);
    int size = (info->info.memory.size);
    return new htn_buffer(remote_addr, size, 0, rkey);
}

// write the information in connect_info into endpoint
void htn_context::SetEndpointInfo(htn_endpoint *endpoint,
                                   struct connect_info *info) {
    switch (endpoint->qp_type_) {
        case IBV_QPT_UD:
            endpoint->dlid_ = info->info.channel.dlid;
            endpoint->remote_sl_ = info->info.channel.sl;
        case IBV_QPT_UC:
        case IBV_QPT_RC:
            endpoint->remote_qpn_ = info->info.channel.qp_num;
            break;
        default:
            LOG(ERROR) << "Currently we don't support other type of QP";
    }
}

// write the information of endpoint into the connection info
void htn_context::GetEndpointInfo(htn_endpoint *endpoint,
                                   struct connect_info *info) {
    memset(info, 0, sizeof(connect_info));
    info->type = (kChannelInfoKey);
    switch (endpoint->qp_type_) {
        case IBV_QPT_UD:
            info->info.channel.dlid = lid_;
            info->info.channel.sl = sl_;
        case IBV_QPT_UC:
        case IBV_QPT_RC:
            info->info.channel.qp_num = endpoint->qp_->qp_num;
            break;
        default:
            LOG(ERROR) << "Currently we don't support other type of QP";
    }
}

// connection request, launched by the client
int htn_context::Connect(const char *server, int port, int connid) {
    int sockfd = -1;
    for (int i = 0; i < kMaxConnRetry; i++) {
        sockfd = ConnectionSetup(server, port);
        if (sockfd > 0) break;
        LOG(INFO) << "Try connect to " << server << ":" << port << " failed for "
                << i + 1 << " times...";
        sleep(1);
    }
    if (sockfd < 0) return -1;
    htn_endpoint *ep;
    union ibv_gid remote_gid;
    char *conn_buf = (char *)malloc(sizeof(connect_info));
    if (!conn_buf) {
        LOG(ERROR) << "Malloc for metadata failed";
        return -1;
    }
    connect_info *info = (connect_info *)conn_buf;
    int number_of_qp, n = 0, rbuf_id = -1;
    std::vector<htn_buffer *> buffers;

    // exchange host information
    memset(info, 0, sizeof(connect_info));
    info->info.host.number_of_qp = (num_qp_per_host_);
    info->info.host.number_of_mem = (FLAGS_buf_num);
    memcpy(&info->info.host.gid, &local_gid_, sizeof(union ibv_gid));
    if (write(sockfd, conn_buf, sizeof(connect_info)) != sizeof(connect_info)) {
        LOG(ERROR) << "Couldn't send local address";
        n = -1;
        goto out;
    }
    n = read(sockfd, conn_buf, sizeof(connect_info));
    if (n != sizeof(connect_info)) {
        PLOG(ERROR) << "client read";
        LOG(ERROR) << "Read only " << n << "/" << sizeof(connect_info) << " bytes";
        goto out;
    }
    if (info->type != kHostInfoKey) {
        LOG(ERROR) << "The First exchange should be host info";
        goto out;
    }
    number_of_qp = (info->info.host.number_of_qp);
    if (number_of_qp != num_qp_per_host_) {
        LOG(ERROR) << "Receiver does not support " << num_qp_per_host_ << " senders";
        goto out;
    }
    memcpy(&remote_gid, &info->info.host.gid, sizeof(union ibv_gid));

    // exchange memory information
    for (int i = 0; i < FLAGS_buf_num; i++) {
        auto buf = PickNextBuffer(1);
        if (!buf) {
            LOG(ERROR) << "Client using buffer error";
            goto out;
        }
        SetInfoByBuffer(info, buf);
        if (write(sockfd, conn_buf, sizeof(connect_info)) != sizeof(connect_info)) {
            LOG(ERROR) << "Couldn't send " << i << " memory's info";
            goto out;
        }
        n = read(sockfd, conn_buf, sizeof(connect_info));
        if (n != sizeof(connect_info)) {
            PLOG(ERROR) << "Client read";
            LOG(ERROR) << n << "/" << (int)sizeof(connect_info) << ": Read " << i
                        << " mem's info failed";
            goto out;
        }
        if ((info->type) != kMemInfoKey) {
            LOG(ERROR) << "Exchange MemInfo failde. Type received is "
                        << (info->type);
            goto out;
        }
        auto remote_buf = CreateBufferFromInfo(info);
        buffers.push_back(remote_buf);
    }

    // rmem_lock_.lock();
    rbuf_id = remote_mempools_.size();
    remote_mempools_.push_back(buffers);
    // rmem_lock_.unlock();

    // exchange endpoint information
    for (int i = 0; i < num_qp_per_host_; i++) {
        ep = endpoints_[i + connid * num_qp_per_host_];
        GetEndpointInfo(ep, info);
        if (write(sockfd, conn_buf, sizeof(connect_info)) != sizeof(connect_info)) {
            LOG(ERROR) << "Couldn't send " << i << "endpoint's info";
            goto out;
        }
        n = read(sockfd, conn_buf, sizeof(connect_info));
        if (n != sizeof(connect_info)) {
            PLOG(ERROR) << "Client Read";
            LOG(ERROR) << "Read only " << n << "/" << sizeof(connect_info)
                        << " bytes";
            goto out;
        }
        if ((info->type) != kChannelInfoKey) {
            LOG(ERROR) << "Exchange Data Failed. Type Received is " << (info->type)
                        << ", expected " << kChannelInfoKey;
            goto out;
        }
        SetEndpointInfo(ep, info);
        if (ep->Activate(remote_gid)) {
            LOG(ERROR) << "Activate " << i << " endpoint failed";
            goto out;
        }
    }

    // go go go!
    memset(info, 0, sizeof(connect_info));
    info->type = (kGoGoKey);
    if (write(sockfd, conn_buf, sizeof(connect_info)) != sizeof(connect_info)) {
        LOG(ERROR) << "Ask GOGO send failed";
        goto out;
    }
    n = read(sockfd, conn_buf, sizeof(connect_info));
    if (n != sizeof(connect_info)) {
        PLOG(ERROR) << "Client Read";
        LOG(ERROR) << "Read only " << n << " / " << sizeof(connect_info)
                << " bytes";
        goto out;
    }
    if ((info->type) != kGoGoKey) {
        LOG(ERROR) << "Ask to Send failed. Receiver reply with " << (info->type)
                << " But we expect " << kGoGoKey;
        goto out;
    }
    for (int i = 0; i < num_qp_per_host_; i++) {
        auto ep = endpoints_[i + connid * num_qp_per_host_];
        ep->activated_ = true;
        ep->remote_server_ = GidToIP(remote_gid);
        ep->rmem_id_ = rbuf_id;
    }
    close(sockfd);
    free(conn_buf);
    return 0;
out:
    close(sockfd);
    free(conn_buf);
    return -1;
}

// std::vector<htn_request> htn_context::GenerateReq() {
//     std::vector<htn_request> requests;
    // for (int i = 0; i < )
// }

int htn_context::ConnectionSetup(const char *server, int port) {
    struct addrinfo *res, *t;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char *service;
    int n;
    int sockfd = -1;
    int err;
    if (asprintf(&service, "%d", port) < 0) return -1;
    n = getaddrinfo(server, service, &hints, &res);
    if (n < 0) {
        LOG(ERROR) << gai_strerror(n) << " for " << server << ":" << port;
        free(service);
        return -1;
    }
    for (t = res; t; t = t->ai_next) {
        sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
        if (sockfd >= 0) {
            if (!connect(sockfd, t->ai_addr, t->ai_addrlen)) break;
            close(sockfd);
            sockfd = -1;
        }
    }
    freeaddrinfo(res);
    free(service);
    if (sockfd < 0) {
        LOG(ERROR) << "Couldn't connect to " << server << ":" << port;
        return -1;
    }
    return sockfd;
}

void htn_context::SetInfoByBuffer(struct connect_info *info,
                                   htn_buffer *buf) {
    info->type = (kMemInfoKey);
    info->info.memory.size = (buf->size_);
    info->info.memory.remote_K = (buf->remote_key_);
    info->info.memory.remote_addr = (buf->addr_);
    return;
}

// WARNING: why is this way?
std::string htn_context::GidToIP(const union ibv_gid &gid) {
    std::string ip =
        std::to_string(gid.raw[12]) + "." + std::to_string(gid.raw[13]) + "." +
        std::to_string(gid.raw[14]) + "." + std::to_string(gid.raw[15]);
    return ip;
}

int htn_context::ServerLaunch() {
    return 0;
}

int htn_context::ClientLaunch() {

    // parse request

    while (1) {
        for (int i = 0; i < endpoints_.size(); i++) {
            if (endpoints_[i] == nullptr) {
                continue;
            }
            if (endpoints_[i]->activated_ == false) {
                continue;
            }
            if (endpoints_[i]->send_credits_ > test_case[i].write_num + test_case[i].read_num + test_case[i].send_recv_num) {
                continue;
            }
            endpoints_[i]->PostSend(send_mempool_, test_case[i], remote_mempools_[endpoints_[i]->rmem_id_]);
        }
        // poll completion
        for (htn_cq cq : send_cqs_) {
            if (PollEach(cq.cq) < 0) {
                LOG(ERROR) << "PollEach failed!";
                exit(1);
            }
        }
    }
    return 0;
}

int htn_context::PollEach(struct ibv_cq *cq) {
    struct ibv_wc wc[kCqPollDepth];
    int wc_num = 0;
    int total_wc_num = 0;
    do {
        wc_num = ibv_poll_cq(cq, kCqPollDepth, wc);
        if (wc_num < 0) {
            PLOG(ERROR) << "ibv_poll_cq() failed";
            return -1;
        }
        for (int i = 0; i < wc_num; i++) {
            if (wc[i].status != IBV_WC_SUCCESS) {
                LOG(ERROR) << "Got bad completion status with " << wc[i].status;
                return -1;
            }
            htn_endpoint *endpoint = reinterpret_cast<htn_endpoint *>(wc[i].wr_id);
            switch (wc[i].opcode) {
                case IBV_WC_RDMA_WRITE:
                case IBV_WC_RDMA_READ:
                case IBV_WC_SEND:
                    // Client Handle CQE
                    endpoint->SendHandler(&wc[i]);
                    break;
                case IBV_WC_RECV:
                case IBV_WC_RECV_RDMA_WITH_IMM:
                    // Server Handle CQE
                    // endpoint->RecvHandler(&wc[i]);
                    // break;
                    // todo
                default:
                    LOG(ERROR) << "Unknown opcode " << wc[i].opcode;
                    return -1;
            }
        }
        total_wc_num += wc_num;
    } while (wc_num > 0);
    return total_wc_num;
}

}