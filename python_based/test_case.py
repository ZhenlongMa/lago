class test_case:
    class process_param:
        size_list = [64, 1024, 16384]
        qp_num_list = [4, 64, 4096]
        
        def __init__(self, qp_num: int, service_type: str, op: str, msg_size:int, sharing_mr: int):
            self.qp_num = qp_num
            self.service_type = service_type
            self.op = op
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
                result += 1
        return result
    