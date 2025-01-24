class test_config:
    process_num = 0
    # case parameter format (with n processes):
    # <qp_num> <service_type> <op> <msg size> <sharing_mr>
    # service_type: 1-RC; 2-UC; 3-UD
    # op: 1-WRITE; 2-READ; 3-SEND; 4-RECV
    case_param = []

    def __init__(self, platform):
        self.process_num = 0
        if platform == "DPU":
            OBJ_DIR = "/home/mazhenl/shared/rnic_test/simple_script" # for DPU platform

            # Communication parameters
            TEST_TYPE = "ib_write_bw"
            MSG_SZ = 64
            WQE_NUM = 100
            MTU = 4096
            GID_INDEX = "0"
            USER = "mazhenl" # for DPU platform

            # Hosts and devices
            SERVERS = ["10.5.200.186"] * 4 # for DPU platform
            CLIENTS = ["10.5.200.187"] * 4 # for DPU platform
            DEVICES = ["mlx5_0"] * 8  # First 4 are servers, last 4 are clients

        elif platform == "28":
            OBJ_DIR = "/work/mazhenlong/rnic_test/python_based" # for 23/25 platform

            # Communication parameters
            TEST_TYPE = "ib_write_bw"
            MSG_SZ = 64
            WQE_NUM = 100
            MTU = 4096
            GID_INDEX = "0"
            USER = "root" # for 23/25 platform

            # Hosts and devices
            SERVERS = ["192.168.0.23"] * 4 # for 23/25 platform
            CLIENTS = ["192.168.0.25"] * 4 # for 23/25 platform
            DEVICES = ["mlx5_0"] * 8  # First 4 are servers, last 4 are clients
        else:
            raise Exception(f"Illegal platform! {platform}")
    
    def generate_init_case(self):
        self.case_param = ["1 1 1 64 0"]