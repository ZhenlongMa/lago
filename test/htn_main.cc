// MIT License

// Copyright (c) 2021 ByteDance Inc. All rights reserved.
// Copyright (c) 2021 Duke University.  All rights reserved.

// See LICENSE for license information

#include <thread>
#include <htn_helper.hh>

int main(int argc, char **argv) {
    if (Htn::Initialize(argc, argv)) {
        return -1;
    }
    std::thread listen_thread;
    std::thread server_thread;
    ifstream test_file("filename");
    string qp_info;
    std::vector<test_qp> test_case;
    while (getline(test_file, qp_info)) {
        test_qp test;
        stringstream qp_info_stream(qp_info);
        qp_info_stream >> test.service_type;
        qp_info_stream >> test.write_num;
        qp_info_stream >> test.read_num;
        qp_info_stream >> test.send_recv_num;
        qp_info_stream >> test.mr_num;
        qp_info_stream >> test.sg_num;
        qp_info_stream >> test.data_size;
        test_case.push_back(test);
    }
    if (FLAGS_server) {
        htn_context* server_context = new htn_context();
        if (server_context->Init()) {
            LOG(ERROR) << "Server initialization failed!";
            return -1;
        }
        listen_thread = std::thread(&Listen, server_context);
        server_thread = std::thread(&ServerLaunch, server_context);
    }
    else {
        std::vector<string> host_list = ParseHost(FLAGS_connect_ip);
        htn_context* client_context = new htn_context();
        if (client_context->Init()) {
            LOG(ERROR) << "Client initialization failed!";
            return -1;
        }
        for (int i = 0; i < host_list.size(); i++) {
            if (client_context->Connect(host_list[i])) {
                LOG(ERROR) << "Client connect failed!";
                return -1;
            }
        }
        client_context->ClientLaunch();
    }
    listen_thread.join();
    server_thread.join();
    return 0;
}