// MIT License

// Copyright (c) 2021 ByteDance Inc. All rights reserved.
// Copyright (c) 2021 Duke University.  All rights reserved.

// See LICENSE for license information

#include "htn_context.hh"

namespace Htn {

int htn_context::Init() {
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
    device_list = ibv_get_device_list(&n);
    if (!device_list) {
        LOG(ERROR) << "ibv_get_device_list() failed!";
        return -1;
    }
    for (int i = 0; i < dev_num; i++) {
        dev = device_list[i];
        if (!strcmp(ibv_get_device_name(dev), device_name_, strlen(device_name))) {
            flag = true;
            break;
        }
    }
    if (!flag) {
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
    auto num_of_qps = num_of_hosts_ * num_per_host_;
    for (int i = 0; i < num_of_qps; i++) {
        ids_.push(i);
    }
    return num_of_qps;
}

int htn_context::InitMemory() {
    // In default, each MR has a identical PD.
    int pd_num;
    for (int i = 0; i < pd_num; i++) {
        struct ibv_pd *pd = ibv_alloc_pd(ctx_);
        if (!pd) {
            PLOG(ERROR) << "ibv_alloc_pd() failed";
            return -1;
        }
        pds_.push_back(pd);
    }
    int buffer_size = FLAGS_buf_size;
    for (int i = 0; i < mr_num; i++) {
        auto region =
            new rdma_region(GetPd(i), buf_size, FLAGS_buf_num, FLAGS_memalign, 0);
        if (region->Mallocate()) {
            LOG(ERROR) << "Region Memory allocation failed";
            break;
        }
        send_mempool_.push_back(region);
        region =
            new rdma_region(GetPd(i), buf_size, FLAGS_buf_num, FLAGS_memalign, 0);
        if (region->Mallocate()) {
            LOG(ERROR) << "Region Memory allocation failed";
            break;
        }
        recv_mempool_.push_back(region);
    }

    // Allocate CQ
    int cqn = num_of_hosts_ * num_per_host;
    for (int i = 0; i < cqn; i++) {
        union collie_cq send_cq;
        union collie_cq recv_cq;
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
    rdma_endpoint *ep = nullptr;
    int cnt = 0;
    while (!ids_.empty()) {
        int id = ids_.front();
        ids_.pop();
        if (endpoints_[id]) {
            delete endpoints_[id];
        }
        struct ibv_qp_init_attr qp_init_attr = MakeQpInitAttr(
            GetSendCq(id), GetRecvCq(id), FLAGS_send_wq_depth, FLAGS_recv_wq_depth);
        ibv_qp *qp = ibv_create_qp(GetPd(id), &qp_init_attr);
        if (!qp) {
            PLOG(ERROR) << "ibv_create_qp() failed";
            delete ep;
            return -1;
        }
        ep = new rdma_endpoint(id, qp);
        ep->SetMaster(this);
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
            std::thread(&rdma_context::AcceptHandler, this, *connfd);
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
    std::vector<rdma_buffer *> buffers;
    auto reqs = ParseRecvFromStr();
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
    if (num_of_recv_ + number_of_qp > num_per_host_ * num_of_hosts_) { // If client's request is out of server's capacity, 
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

    rmem_lock_.lock();
    remote_mempools_.push_back(buffers);
    rbuf_id = remote_mempools_.size() - 1;
    rmem_lock_.unlock();

    // Get the connection channel info from remote

    for (int i = left; i < right; i++) {
        auto ep = (rdma_endpoint *)endpoints_[i];
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
        int batch_size = FLAGS_recv_batch;
        size_t idx = 0;
        while (ep->GetRecvCredits() > 0) {
            auto num_to_post = std::min(first_batch, batch_size);
            if (ep->PostRecv(reqs, idx, num_to_post)) {
                LOG(ERROR) << "The " << i << " Receiver Post first batch error";
                goto out;
            }
            first_batch -= num_to_post;
        }
        ep->SetActivated(true);
        ep->SetMemId(rbuf_id);
        ep->SetServer(GidToIP(gid));
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

rdma_buffer *htn_context::CreateBufferFromInfo(struct connect_info *info) {
  uint64_t remote_addr = (info->info.memory.remote_addr);
  uint32_t rkey = (info->info.memory.remote_K);
  int size = (info->info.memory.size);
  return new rdma_buffer(remote_addr, size, 0, rkey);
}

}