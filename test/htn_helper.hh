#include <gflags/gflags.h>
#include <glog/logging.h>

#include <infiniband/mlx5dv.h>
#include <infiniband/verbs.h>

DECLARE_string(dev);
DECLARE_int32(gid);

DECLARE_bool(server);
DECLARE_string(traffic);

namespace Htn {

class connect_info {
public:
    int type;
    union {
        struct {
            union ibv_gid gid;
            int number_of_qp;
            int number_of_mem;
        } host;
        struct {
            uint64_t remote_addr;
            uint32_t remote_K;  // K short for Key because key is a sensitive word due
                                // to NDA :)
            int size;
        } memory;
        struct {
            int qp_num;  // QP number. For connection setup
            uint16_t dlid;
            uint8_t sl;
        } channel;
    } info;
};

int Initialize(int argc, char **argv);

}