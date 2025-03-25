import case_driver
import test_config
import analyzer
import test_case
import copy
import random
import time
import re

class iterator:
    def __init__(self, config: test_config, driver: case_driver, analyzer: analyzer):
        self.config = config
        self.driver = driver
        self.reach_destination = False
        self.analyzer = analyzer
        self.anomaly_id = self.analyzer.max_anomaly_index + 1
        self.anomaly_case = []

    def record_case_throughput(self, case: test_case.test_case, throughput: float):
        print(f"anomaly ID: {self.anomaly_id}, throughput: {throughput}")
        anomaly_file = open(f"anomaly_{self.anomaly_id}", "w")
        anomaly_file.write(f"Throughput: {throughput}\n")
        anomaly_file.write(f"anomaly process id: {case.new_proc_id}\n")
        anomaly_file.write(f"fatal param: ")
        for i in range(len(case.fatal_param)):
            anomaly_file.write(f"{case.fatal_param[i]} ")
        anomaly_file.write(f"\n")
        anomaly_file.write(f"******************************\n")
        anomaly_file.write(f"Case Parameter:\n")
        anomaly_file.write("qp_num\tsvc_type\top\t\tmsg_size\tsharing_mr\n")
        for i in range(len(case.param)):
            anomaly_file.write(f"{case.param[i].qp_num}\t\t{case.param[i].service_type}\t\t\t"
                               f"{case.param[i].op}\t{case.param[i].msg_size}\t{case.param[i].sharing_mr}\n")
        anomaly_file.close()
        self.anomaly_id += 1

    # if all processes' QP number is positive or -2, the test reaches the end because each process is 
    # either tested or abandoned
    def reach_end(self, case, terminus):
        for param in case.param:
            assert param.qp_num != -1
            if param.qp_num == 0:
                return False
        return True

    def launch_test(self):
        self.read_history_anomalies()
        # in the whole test, iterate for several times
        for i in range(self.config.round_num):
            print(f"start round {i}")
            case = self.set_start_case()
            case = self.set_next_case(case, self.config.terminus)
            while True: # if the test case doesn't exceed terminus
                if case == None:
                    break
                print(f"case valid qp num: {case.count_valid_qp_num()}, terminus valid qp num: {self.config.terminus.count_valid_qp_num()}")
                assert case.count_valid_qp_num() <= self.config.terminus.count_valid_qp_num()
                self.driver.test(case) # run test and generate test_result_xx files
                throughput = self.analyzer.calculate_throughput(case)
                print(f"throughput: {throughput}")
                if throughput < 0.8:
                    print(f"throughput degradation! search for anoamly factor!")
                    
                    # determine the factor leading to the anomaly
                    try_case = copy.deepcopy(case)
                    anomaly_case = copy.deepcopy(case)

                    # iteratively change the parameter and test the throughput, if a parameter does not cause the throughput degradation,
                    # label it as -1 indicating that this parameter is irrelavant

                    # change qp num
                    # 4 -> 4096
                    # 64 -> 4096
                    # 4096 -> 4
                    # try_case.param[try_case.new_proc_id].qp_num = try_case.param[try_case.new_proc_id].qp_num / 2
                    if try_case.param[try_case.new_proc_id].qp_num == 4:
                        try_case.param[try_case.new_proc_id].qp_num = 4096
                    elif try_case.param[try_case.new_proc_id].qp_num == 64:
                        try_case.param[try_case.new_proc_id].qp_num = 4096
                    elif try_case.param[try_case.new_proc_id].qp_num == 4096:
                        try_case.param[try_case.new_proc_id].qp_num = 64
                    self.driver.test(try_case)
                    try_throughput = self.analyzer.calculate_throughput(try_case)
                    print(f"throughput: {try_throughput}")
                    # if throughput is still less than 0.8, qp num is not the critical factor
                    if try_throughput < 0.8:
                        anomaly_case.param[anomaly_case.new_proc_id].qp_num = -1
                    else:
                        anomaly_case.fatal_param.append(0)

                    # change msg size
                    # 64 -> 16K
                    # 1K -> 16K
                    # 16K -> 64
                    if try_case.param[try_case.new_proc_id].msg_size == 64:
                        try_case.param[try_case.new_proc_id].msg_size = 16384
                    elif try_case.param[try_case.new_proc_id].msg_size == 1024:
                        try_case.param[try_case.new_proc_id].msg_size = 16384
                    elif try_case.param[try_case.new_proc_id].msg_size == 16384:
                        try_case.param[try_case.new_proc_id].msg_size = 64
                    self.driver.test(try_case)
                    try_throughput = self.analyzer.calculate_throughput(try_case)
                    print(f"throughput: {try_throughput}")
                    # if throughput is still less than 0.8, message size is not the critical factor
                    if try_throughput < 0.8:
                        anomaly_case.param[anomaly_case.new_proc_id].msg_size = -1
                    else:
                        anomaly_case.fatal_param.append(3)

                    # change sharing mr
                    if try_case.param[try_case.new_proc_id].sharing_mr == 0:
                        try_case.param[try_case.new_proc_id].sharing_mr = 1
                        try_throughput = self.analyzer.calculate_throughput(try_case)
                        if try_throughput < 0.8:
                            anomaly_case.param[anomaly_case.new_proc_id].sharing_mr = -1
                    else:
                        anomaly_case.param[anomaly_case.new_proc_id].sharing_mr = -1

                    if try_case.param[try_case.new_proc_id].op == "WRITE":
                        try_case.param[try_case.new_proc_id].op = "READ"
                    elif try_case.param[try_case.new_proc_id].op == "READ":
                        try_case.param[try_case.new_proc_id].op = "WRITE"
                    try_throughput = self.analyzer.calculate_throughput(try_case)
                    if try_throughput < 0.8:
                        anomaly_case.param[anomaly_case.new_proc_id].op = "ANY"

                    self.anomaly_case.append(anomaly_case)
                    self.record_case_throughput(anomaly_case, throughput)

                if self.reach_end(case, self.config.terminus):
                    break
                case = self.set_next_case(case, self.config.terminus)
                # if case == None:
                #     break

    # cancel the process leading to an anomaly
    def mutate_process(self, param_index):
        self.config.terminus.param[param_index].qp_num = 0

    def set_start_case(self):
        start_case = test_case.test_case()
        # start_process_num = 1

        # to do: generate the start case
        for i in range(len(self.config.terminus.param)):
            # param = start_case.process_param(1, "RC", "WRITE", 64, False)
            # param = copy.deepcopy(self.config.terminus)
            param = start_case.process_param(0, self.config.terminus.param[i].service_type, self.config.terminus.param[i].op, self.config.terminus.param[i].msg_size, 0)
            start_case.param.append(param)

        # temp setting: copy the terminus qp num
        # qp_num = self.config.terminus.param[0].qp_num
        # start_case.param[0].qp_num = qp_num
        # start_case.param[0].msg_size = self.config.terminus.param[0].msg_size
        # start_case.param[0].sharing_mr = self.config.terminus.param[0].sharing_mr
        # start_case.new_proc_id = 0
        return start_case

    # set the next case to run according to the current case, the original case, the final case, 
    # and the throughput anomaly
    def set_next_case(self, current_case, final_case):

        # if no anomaly is detected, randomly choose a process from the final case
        if len(self.anomaly_case) == 0:
            print("No anomaly found! Random pick!")
            next_case = copy.deepcopy(current_case)
            invalid_process_index = []
            for i in range(len(current_case.param)):
                if current_case.param[i].qp_num == 0:
                    invalid_process_index.append(i)
            if len(invalid_process_index) == 0:
                return None
            else:
                random_process = random.choice(invalid_process_index)
                next_case.param[random_process] = copy.deepcopy(final_case.param[random_process])
                next_case.new_proc_id = random_process
                return next_case
        else:
            print(f"Found {len(self.anomaly_case)} anomaly records!")
            while 1:
                next_case = copy.deepcopy(current_case)
                # indexed according to the final case
                distance_to_anomaly = []
                # calculate each candidate's distance to anomaly
                for i in range(len(current_case.param)):
                    if current_case.param[i].qp_num == 0:
                        dist = -1
                        current_case.param[i].qp_num = final_case.param[i].qp_num
                        for j in range(len(self.anomaly_case)):
                            assert current_case.param[i].qp_num != 0
                            if dist == -1:
                                dist = self.analyzer.case_diff(current_case, self.anomaly_case[j])
                            else:
                                dist = min(dist, self.analyzer.case_diff(current_case, self.anomaly_case[j]))
                            assert dist >= 0
                        distance_to_anomaly.append(dist)
                    else:
                        distance_to_anomaly.append(-1)
                    current_case.param[i].qp_num = 0
                assert len(distance_to_anomaly) > 0
                print(distance_to_anomaly)
                max_dist = max(distance_to_anomaly)
                print(f"max distance: {max_dist}")
                max_index = distance_to_anomaly.index(max_dist)
                assert current_case.param[max_index].qp_num == 0
                next_case.param[max_index] = copy.deepcopy(final_case.param[max_index])
                print(f"next case:")
                next_case.print_case_info()
                next_case.new_proc_id = max_index
                current_case = copy.deepcopy(next_case)
                if max_dist != 0:
                    assert max_dist > 0
                    return next_case
                elif self.reach_end(next_case, self.config.terminus):
                    return None

            # print(f"Found {len(self.anomaly_case)} anomaly records!")
            # next_case = copy.deepcopy(current_case)
            # # indexed according to the final case
            # distance_to_anomaly = []
            # # calculate each candidate's distance to anomaly
            # for i in range(len(current_case.param)):
            #     if current_case.param[i].qp_num == 0:
            #         dist = -1
            #         current_case.param[i].qp_num = final_case.param[i].qp_num
            #         for j in range(len(self.anomaly_case)):
            #             assert current_case.param[i].qp_num != 0
            #             if dist == -1:
            #                 dist = self.analyzer.case_diff(current_case, self.anomaly_case[j])
            #             else:
            #                 dist = min(dist, self.analyzer.case_diff(current_case, self.anomaly_case[j]))
            #             assert dist >= 0
            #         distance_to_anomaly.append(dist)
            #     else:
            #         distance_to_anomaly.append(-1)
            #     current_case.param[i].qp_num = 0
            # assert len(distance_to_anomaly) > 0
            # print(distance_to_anomaly)
            # max_dist = max(distance_to_anomaly)
            # print(f"max distance: {max_dist}")
            # max_index = distance_to_anomaly.index(max_dist)
            # assert current_case.param[max_index].qp_num == 0
            # next_case.param[max_index] = copy.deepcopy(final_case.param[max_index])
            # print(f"next case:")
            # next_case.print_case_info()
            # next_case.new_proc_id = max_index
            # return next_case

        
    def read_history_anomalies(self):
        anomaly_path = "./"
        for anomaly_id in range(self.anomaly_id):
            filename = f"anomaly_{anomaly_id}"
            with open(filename, "r") as f:
                next_active = 0
                existing_anomaly_case = test_case.test_case()
                print(f"open {filename}")
                for line in f.readlines():
                    line = line.strip()
                    # line_list = line.split(' ')
                    line_list = re.split(r'[ \t]+', line)
                    # print(line_list)
                    if next_active == 1:
                        # anomaly param format: qp_num svc_type op msg_size sharing_mr
                        param = existing_anomaly_case.process_param(int(line_list[0]), line_list[1], line_list[2], int(line_list[3]), int(line_list[4]))
                        existing_anomaly_case.param.append(param)
                    if line_list[0] == "qp_num":
                        next_active = 1
                if next_active == 0:
                    raise Exception("Empty anomaly file!")
                self.anomaly_case.append(existing_anomaly_case)
            # raise Exception("temp")