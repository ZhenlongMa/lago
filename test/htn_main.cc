// MIT License

// Copyright (c) 2021 ByteDance Inc. All rights reserved.
// Copyright (c) 2021 Duke University.  All rights reserved.

// See LICENSE for license information

#include <thread>
#include <htn_context.hh>
#include <htn_helper.hh>
#include <stdlib>

int main(int argc, char **argv) {
    if (Htn::Initialize(argc, argv)) {
        return -1;
    }
    std::thread listen_thread;
    std::thread server_thread;
    ifstream TestFile("filename");
    string qpInfo;
    while (getline(TestFile, qpInfo)) {
        
    }
    if (FLAGS_server) {
        rdma_context* server_context = new rdma_context();
        if (server_context->Init()) {
            LOG(ERROR) << "Server initialization failed!";
            return -1;
        }
        listen_thread = std::thread(&Listen, server_context);
        server_thread = std::thread(&ServerLaunch, server_context);
    }
    else {
        std::vector<string> host_list = ParseHost(FLAGS_connect_ip);
        rdma_context* client_context = new rdma_context();
        if (client_context->Init()) {
            LOG(ERROR) << "Client initialization failed!";
            return -1;
        }
        for (int i = 0; i < host_list.size(); i++) {
            todo
        }
    }
    listen_thread.join();
    server_thread.join();
    return 0;
}