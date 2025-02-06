import test_config
import test_case

class analyzer:
    def __init__(self, config):
        self.config = config

    # calculate the throughput of the current case.
    # the input is the case
    def calculate_throughput(self):
        # read test result file

        # calculate bdp and expected bdp

        # clculate throughput

        # map case to throughput
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


    def parse_file(file_name, msg_sz):
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