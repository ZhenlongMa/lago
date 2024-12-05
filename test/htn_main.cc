// MIT License

// Copyright (c) 2021 ByteDance Inc. All rights reserved.
// Copyright (c) 2021 Duke University.  All rights reserved.

// See LICENSE for license information

#include <thread>
// #include <string>
#include "htn_context.hh"

int main(int argc, char **argv) {
    if (Htn::Initialize(argc, argv)) {
        return -1;
    }
    std::thread listen_thread;
    std::thread server_thread;
    
    if (FLAGS_server) { // server branch
        Htn::htn_context* server_context = new Htn::htn_context();
        if (server_context->Init()) {
            LOG(ERROR) << "Server initialization failed!";
            return -1;
        }
        // The listen thread continuously monitors the network data.
        // listen_thread = std::thread(&Listen);
        // server_thread = std::thread(&ServerLaunch);
    }
    else { // client branch
        std::vector<std::string> host_list = Htn::ParseHost(FLAGS_connect_ip);
        Htn::htn_context* client_context = new Htn::htn_context();
        if (client_context->Init()) {
            LOG(ERROR) << "Client initialization failed!";
            return -1;
        }
        for (int i = 0; i < host_list.size(); i++) {
            // if (client_context->Connect(host_list[i])) {
            //     LOG(ERROR) << "Client connect failed!";
            //     return -1;
            // }
        }
        // client_context->ClientLaunch();
    }
    listen_thread.join();
    server_thread.join();
    return 0;
}