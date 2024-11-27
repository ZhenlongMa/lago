#include <thread>
#include <test_context.hh>
#include <helper.hh>
#include <stdlib>

int main(int argc, char **argv) {
    if (Htn::Initialize(argc, argv)) {
        return -1;
    }
    ifstream TestFile("filename");
    string qpInfo;
    while (getline(TestFile, qpInfo)) {
        
    }
    if (FLAGS_server) {

    }
    else {

    }
    listen_thread.join();
    server_thread.join();
    return 0;
}