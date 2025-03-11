import test_case

class test_config:
    process_num = 0
    # case parameter format (with n processes):
    # <qp_num> <service_type> <op> <msg size> <sharing_mr>
    # service_type: 1-RC; 2-UC; 3-UD
    # op: 1-WRITE; 2-READ; 3-SEND; 4-RECV
    case_param_vector = []

    case_param = []

    class param:
        def __init__(self, qp_num: int, service_type: str, op:str, msg_size:int, sharing_mr: int):
            self.qp_num = qp_num
            self.service_type = service_type
            self.op = op
            self.msg_size = msg_size
            self.sharing_mr = sharing_mr

    class destination:
        def __init__(self):
            self.msg_size = 1048576
            self.qp_num = 64 # total QP number

    def __init__(self, platform):
        self.round_num = 1 # test for five rounds
        self.dst = self.destination()
        self.process_num = 0
        self.generate_terminus()
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

            self.peak_bdp = 100 # Gbps

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

            self.peak_bdp = 100 # Gbps
        else:
            raise Exception(f"Illegal platform! {platform}")
    
    # def generate_init_case(self):
    #     self.case_param_vector = ["1 1 1 64 0"]
    #     test_param = self.param(1, "RC", "WRITE", 64, True)
    #     self.case_param.append(test_param)

    # generate the boundary of the test and save as a case
    # todo: change this version to be more heterogeneous and randomized
    def generate_terminus(self):
        terminus_process_num = 7
        self.terminus = test_case.test_case()
        # todo: server side launch
        for i in range(3):
            param = self.terminus.process_param(qp_num = 4, service_type = "RC", op = "READ", \
                                                msg_size = 65536, sharing_mr = 1)
            self.terminus.param.append(param)
        for i in range(2):
            param = self.terminus.process_param(qp_num = 4, service_type = "RC", op = "WRITE", \
                                                msg_size = 64, sharing_mr = 1)
            self.terminus.param.append(param)
        for i in range(2):
            param = self.terminus.process_param(qp_num = 4, service_type = "RC", op = "READ", \
                                                msg_size = 64, sharing_mr = 1)
            self.terminus.param.append(param)