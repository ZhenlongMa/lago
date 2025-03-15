class test_case:
    class process_param:
        size_list = [64, 1024, 16384]
        qp_num_list = [4, 64, 4096]
        
        def __init__(self, qp_num: int, service_type: str, op: str, msg_size:int, sharing_mr: int, bidirection = 0):
            # qp_num == -1 means any QP number
            # qp_num == -2 means abandoned param
            self.qp_num = qp_num
            # available: RC, UC, UD, ANY
            self.service_type = service_type
            # available: WRITE, READ, ANY
            self.op = op
            # msg_size == -1 means any message size
            self.msg_size = msg_size
            self.sharing_mr = sharing_mr
            
        # def __init__(self, service_type: str, op: str):
        #     self.qp_num = 0
        #     self.service_type = service_type
        #     self.op = op
        #     self.msg_size = 0
        #     self.sharing_mr = False

    def __init__(self):
        # todo
        self.process_num = 0
        self.param = [] # store for each process
        self.total_qp_num = 0
        self.new_proc_id = 0

    def get_active_process_num(self):
        result = 0
        for i in range(len(self.param)):
            if self.param[i].qp_num != 0:
                assert self.param[i].qp_num != -1
                result += 1
        return result
    
    def count_total_qp_num(self):
        result = 0
        for param in self.param:
            assert param.qp_num >= 0
            result += param.qp_num
        return result
    
    def count_valid_qp_num(self):
        result = 0
        for param in self.param:
            if param.qp_num >= 0:
                result += param.qp_num
        return result