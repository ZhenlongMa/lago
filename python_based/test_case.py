class test_case:
    class process_param:
        def __init__(self, qp_num: int, service_type: str, op: str, msg_size:int, sharing_mr: bool):
            self.qp_num = qp_num
            self.service_type = service_type
            self.op = op
            self.msg_size = msg_size
            self.sharing_mr = sharing_mr
            
        def __init__(self, service_type: str, op: str):
            self.qp_num = 0
            self.service_type = service_type
            self.op = op
            self.msg_size = 0
            self.sharing_mr = False

    def __init__(self):
        # todo
        self.process_num = 0
        self.param = [] # store for each process
        self.total_qp_num = 0

    def get_active_process_num(self):
        result = 0
        for i in range(self.process_num):
            if self.param[i].qp_num != 0:
                result += 1
        return result
    