class test_config:
    # OBJ_DIR = "/home/mazhenl/shared/rnic_test/simple_script" # for DPU platform
    OBJ_DIR = "/work/mazhenlong/rnic_test/python_based" # for 23/25 platform

    # Communication parameters
    TEST_TYPE = "ib_write_bw"
    MSG_SZ = 64
    WQE_NUM = 100
    MTU = 4096
    GID_INDEX = "0"
    # USER = "mazhenl" # for DPU platform
    USER = "root" # for 23/25 platform

    # Hosts and devices
    # SERVERS = ["10.5.200.186"] * 4 # for DPU platform
    # CLIENTS = ["10.5.200.187"] * 4 # for DPU platform
    SERVERS = ["192.168.0.23"] * 4 # for 23/25 platform
    CLIENTS = ["192.168.0.25"] * 4 # for 23/25 platform
    DEVICES = ["mlx5_0"] * 8  # First 4 are servers, last 4 are clients
    process_num = 0
    case_param = []

    def __init__(self):
        self.process_num = 0

    def generate_init_case(self):
        