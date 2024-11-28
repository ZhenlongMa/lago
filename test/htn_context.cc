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
        
    }
}

}