import test_config
import test_case
import copy

class analyzer:
    def __init__(self, config):
        self.config = config

    # calculate the throughput of the current case.
    # the input is the case
    def calculate_throughput(self, case):
        total_qp_num = 0
        self.process_pps_vec = []
        self.process_bps_vec = []
        self.qp_bps_vec = []
        self.qp_pps_vec = []
        for i in range(len(case.param)):
            if case.param[i].qp_num != 0:
                msg_rate = self.parse_file_msg_rate(f"test_result_c{i}", case.param[i].msg_size)
                self.process_pps_vec.append(msg_rate)
                self.process_bps_vec.append(msg_rate * (case.param[i].msg_size + 64) * 8) # unit: Mbps
                total_qp_num += case.param[i].qp_num
            else:
                self.process_pps_vec.append(0)
                self.process_bps_vec.append(0)

        for i in range(len(case.param)):
            if case.param[i].qp_num == 0:
                continue
            else:
                for j in range(case.param[i].qp_num):
                    self.qp_bps_vec.append(self.process_bps_vec[i] / case.param[i].qp_num)
                    self.qp_pps_vec.append(self.process_pps_vec[i] / case.param[i].qp_num)
        
        # calculate the cdf according to bps
        max_bps = 100000.00 # Mbps
        max_pps = 64000.00 # Mpps
        bias = 0
        for i in range(len(self.qp_bps_vec)):
            # todo: decide whether using bps or pps
            if self.qp_bps_vec[i] > max_bps / len(self.qp_bps_vec):
                bias += 0
            else:
                expected = max_bps / len(self.qp_bps_vec)
                print(f"max_bps: {max_bps}, length: {len(self.qp_bps_vec)}, bps: {self.qp_bps_vec[i]}")
                bias += (max_bps / len(self.qp_bps_vec) - self.qp_bps_vec[i]) / len(self.qp_bps_vec) / (max_bps / len(self.qp_bps_vec))
        return 1 - bias

        # map the throughput to the case
        # todo

    def judge_anomaly(self, case):
        if self.calculate_throughput(case) < 0.8:
            return True

    # if all parameters in case 1 exceeds case 2, return True
    def case_larger(self, case1: test_case, case2: test_case):
        # todo
        if len(case1.param) < len(case2.param):
            return False
        else:
            for i in range(len(case1.param)):
                if case1.param[i].qp_num < case2.param[i].qp_num:
                    return False
                if case1.param[i].msg_size < case2.param[i].msg_size:
                    return False
                if case1.param[i].sharing_mr == 0 and case2.param[i].sharing_mr == 1:
                    return False
        return True

    # calculate the difference between two cases
    def case_diff(self, case1: test_case, case2: test_case):
        def compress_case(self, temp_case1):
            for i in range(len(temp_case1.param)):
                if temp_case1.param[i].qp_num != 0:
                    for j in range(i + 1, len(temp_case1.param)):
                        if temp_case1.param[j].qp_num != 0:
                            if temp_case1.param[j].service_type == temp_case1.param[i].service_type and \
                                temp_case1.param[j].op == temp_case1.param[i].op and \
                                temp_case1.param[j].msg_size == temp_case1.param[i].msg_size:
                                if temp_case1.param[j].qp_num == -1 or temp_case1.param[i].qp_num == -1:
                                    temp_case1.param[i].qp_num = -1
                                    temp_case1.param[j].qp_num = 0
                                else:
                                    temp_case1.param[i].qp_num += temp_case1.param[j].qp_num
                                    temp_case1.param[j].qp_num = 0

        # compress
        temp_case1 = copy.deepcopy(case1)
        temp_case2 = copy.deepcopy(case2)
        compress_case(temp_case1)
        compress_case(temp_case2)

        # calculate distance
        same_workload_num = 0
        total_qp_num_1 = 0
        total_qp_num_2 = 0
        for param1 in temp_case1.param:
            assert param1.qp_num != -1
            total_qp_num_1 += param1.qp_num
        for param2 in temp_case2.param:
            if param2.qp_num != -1:
                total_qp_num_2 += param2.qp_num
        for param1 in temp_case1.param:
            if param1.qp_num != 0:
                assert param1.qp_num > 0
                assert param1.op != "ANY"
                assert param1.service_type != "ANY"
                assert param1.msg_size != -1
                for param2 in temp_case2.param:
                    if param2.qp_num != 0:
                        # modify here
                        if (param1.op == param2.op or param2.op == "ANY") and \
                            (param1.msg_size == param2.msg_size or param2.msg_size == -1) and \
                            (param1.service_type == param2.service_type or param2.service_type == "ANY"):
                            if param2.qp_num == -1:
                                total_qp_num_2 += param1.qp_num
                                same_workload_num += param1.qp_num
                            else:
                                same_workload_num += min(param1.qp_num, param2.qp_num)
                            
        return same_workload_num / ((total_qp_num_1 + total_qp_num_2) / 2)

    # calculate the message rate
    def parse_file_msg_rate(self, file_name, msg_sz):
        res = [] 
        with open(file_name, "r", encoding="utf-8") as f:
            for line in f.readlines():
                line = line.strip()
                line_list = line.split(' ')
                # print("line_list[0]: " + line_list[0])
                if line_list[0] == str(msg_sz):
                    if len(line_list[-1]) > 8:
                        mrate = line_list[-1][0:7]
                    else:
                        mrate = line_list[-1]
                    line_list[-1].strip()
                    # print("res for every line: " + line_list[-1])
                    res.append(float(line_list[-1]))
        if len(res) == 0:
            raise Exception(f"no result in {file_name}!")
        else:
            return (sum(res) / len(res))
