#include <helper.hh>

int Initialize(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;
    // parse parameters into FLAGS_<DECLARE_xxx>
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return 0;
}