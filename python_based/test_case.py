class test_case:
    class process_param:
        def __init__(self, qp_num: int, service_type: str, op: str, msg_size:int, sharing_mr: bool):
            self.qp_num = qp_num
            self.service_type = service_type
            self.op = op
            self.msg_size = msg_size
            self.sharing_mr = sharing_mr
    def __init__(self):
        # todo
        self.process_num = 0
        case = [] # store for each process