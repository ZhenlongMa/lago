class test_config:
    process_num = 0
    # case parameter format (with n processes):
    # <qp_num> <service_type> <op> <msg size> <sharing_mr>
    # service_type: 1-RC; 2-UC; 3-UD
    # op: 1-WRITE; 2-READ; 3-SEND; 4-RECV
    case_param_vector = []

    case_param = []

    class param:
        def __init__(self, qp_num: int, service_type: str, op:str, msg_size:int, sharing_mr: bool):
            self.qp_num = qp_num
            self.service_type = service_type
            self.op = op
            self.msg_size = msg_size
            self.sharing_mr = sharing_mr

    def __init__(self, platform):
        self.process_num = 0
        if platform == "DPU":
            self.object_directory = "/home/mazhenl/shared/rnic_test/python_based" # for DPU platform

            # Communication parameters
            self.test_type = "ib_write_bw"
            self.wqe_num = 100
            self.mtu = 4096
            self.gid_index = "0"
            self.user = "mazhenl" # for DPU platform

            # Hosts and devices
            self.servers = ["10.5.200.186"] * 4 # for DPU platform
            self.clients = ["10.5.200.187"] * 4 # for DPU platform
            self.server_devices = ["mlx5_0"] * 8  # First 4 are servers, last 4 are clients
            self.client_devices = ["mlx5_0"] * 8

        elif platform == "28":
            self.object_directory = "/work/mazhenlong/rnic_test/python_based" # for 23/25 platform

            # Communication parameters
            self.test_type = "ib_write_bw"
            self.wqe_num = 100
            self.mtu = 4096
            self.gid_index = "0"
            self.user = "root" # for 23/25 platform

            # Hosts and devices
            self.servers = ["192.168.0.23"] * 4 # for 23/25 platform
            self.clients = ["192.168.0.25"] * 4 # for 23/25 platform
            self.server_devices = ["mlx5_0"] * 8  # First 4 are servers, last 4 are clients
            self.client_devices = ["mlx5_0"] * 8
        else:
            raise Exception(f"Illegal platform! {platform}")
    
    def generate_init_case(self):
        self.case_param_vector = ["1 1 1 64 0"]
        test_param = self.param(1, "RC", "WRITE", 64, True)
        self.case_param.append(test_param)