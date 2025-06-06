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
        LOG(INFO) << "enter server!";
        if (server_context->Init()) {
            LOG(ERROR) << "Server initialization failed!";
            return -1;
        }
        std::cout << "cout: server init finish!" << std::endl;
        LOG(INFO) << "server init finish!";
        // The listen thread continuously monitors the network data.
        listen_thread = std::thread(&Htn::htn_context::Listen, server_context);
        server_thread = std::thread(&Htn::htn_context::ServerLaunch, server_context);
    }
    else { // client branch
        LOG(INFO) << "enter client!";
        std::cout << "cout enter client!" << std::endl;
        std::vector<std::string> host_list = Htn::ParseHost(FLAGS_connect_ip);
        LOG(INFO) << "parse host finish!";
        Htn::htn_context* client_context = new Htn::htn_context();
        if (client_context->Init()) {
            LOG(ERROR) << "Client initialization failed!";
            return -1;
        }
        LOG(INFO) << "client initialization finish!";
        for (int i = 0; i < host_list.size(); i++) {
            if (client_context->Connect(host_list[i].c_str(), FLAGS_port, i)) {
                LOG(ERROR) << "Client connect failed!";
                return -1;
            }
        }
        LOG(INFO) << "client connect finish!";
        client_context->ClientLaunch();
    }
    listen_thread.join();
    server_thread.join();
    return 0;
}
