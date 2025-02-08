import test_config
import test_case

class analyzer:
    def __init__(self, config):
        self.config = config

    # calculate the throughput of the current case.
    # the input is the case
    def calculate_throughput(self, case):
        total_qp_num = 0
        self.pps_vec = []
        self.bps_vec = []
        for i in range(case.param.size()):
            if case.param[i].qp_num != 0:
                msg_rate = self.parse_file_msg_rate(f"test_result_c{i}")
                self.msg_rate_vec.append(msg_rate)
                self.bps_vec.append(msg_rate * (case.param[i].msg_size + 64)) # unit: Mbps
                total_qp_num += case.param[i].qp_num
            else:
                self.pps_vec.append(0)
                self.bps_vec.append(0)

        # calculate bps and expected bps, generate a CDF

        # calculate the throughput

        # map the throughput to the case
        # todo

    def judge_anomaly(self):
        # todo

    # calculate case 1 - case 2
    # if all parameters in case 1 exceeds case 2, return True
    def diff_case(self, case1: test_case, case2: test_case):
        # todo
        if case1.process_num < case2.process_num:
            return False
        else:
            for i in range(case1.process_num):
                if case1.param[i].qp_num < case2.param[i].qp_num:
                    return False
                if case1.param[i].msg_size < case2.param[i].msg_size:
                    return False
                if case1.param[i].sharing_mr == False and case2.param[i].sharing_mr == True:
                    return False
        return True

    # calculate the message rate
    def parse_file_msg_rate(file_name, msg_sz):
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
            return 0
        else:
            return (sum(res) / len(res))