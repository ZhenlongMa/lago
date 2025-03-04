import case_driver
import test_config
import analyzer
import test_case
import copy
import random
import time

class iterator:
    def __init__(self, config: test_config, driver: case_driver, analyzer: analyzer.analyzer):
        self.config = config
        self.driver = driver
        self.reach_destination = False
        self.analyzer = analyzer
        self.anomaly_id = 0
        self.anomaly_case = []

    def record_case_throughput(self, case: test_case.test_case, throughput: float):
        print(f"anomaly ID: {self.anomaly_id}, throughput: {throughput}")
        anomaly_file = open(f"anomaly_{self.anomaly_id}", "w")
        anomaly_file.write(f"Throughput: {throughput}\n")
        anomaly_file.write(f"******************************\n")
        anomaly_file.write(f"Case Parameter:\n")
        anomaly_file.write("qp_num\tsvc_type\top\t\tmsg_size\tsharing_mr\n")
        for i in range(len(case.param)):
            anomaly_file.write(f"{case.param[i].qp_num}\t\t{case.param[i].service_type}\t\t\t"
                               f"{case.param[i].op}\t{case.param[i].msg_size}\t{case.param[i].sharing_mr}\n")
        anomaly_file.close()
        self.anomaly_id += 1

    def launch_test(self):
        # in the whole test, iterate for several times
        for i in range(self.config.round_num):
            print(f"start round {i}")
            start_case = self.set_start_case()
            case = start_case
            while self.analyzer.case_larger(case, self.config.terminus) != True: # if the test case doesn't exceed terminus
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

                    # change sharing mr
                    if try_case.param[try_case.new_proc_id].sharing_mr == 0:
                        try_case.param[try_case.new_proc_id].sharing_mr = 1
                        try_throughput = self.analyzer.calculate_throughput(try_case)
                        if try_throughput < 0.8:
                            anomaly_case.param[anomaly_case.new_proc_id].sharing_mr = -1
                    else:
                        anomaly_case.param[anomaly_case.new_proc_id].sharing_mr = -1
                    self.anomaly_case.append(anomaly_case)
                    self.record_case_throughput(case, throughput)

                    # mutate the case
                    self.mutate_process(anomaly_case.new_proc_id)

                case = self.set_next_case(case, start_case, self.config.terminus)
                if case == None:
                    break

    # cancel the process leading to an anomaly
    def mutate_process(self, param_index):
        self.config.terminus.param[param_index].qp_num = 0

    def set_start_case(self):
        start_case = test_case.test_case()
        # start_process_num = 1

        # to do: generate the start case
        for i in range(len(self.config.terminus.param)):
            # param = start_case.process_param(1, "RC", "WRITE", 64, False)
            param = start_case.process_param(0, self.config.terminus.param[i].service_type, self.config.terminus.param[i].op, 0, False)
            start_case.param.append(param)

        # temp setting: copy the terminus qp num
        qp_num = self.config.terminus.param[0].qp_num
        start_case.param[0].qp_num = qp_num
        start_case.param[0].msg_size = self.config.terminus.param[0].msg_size
        start_case.param[0].sharing_mr = self.config.terminus.param[0].sharing_mr
        start_case.new_proc_id = 0
        return start_case

    # set the next case to run according to the current case, the original case, the final case, 
    # and the throughput anomaly
    def set_next_case(self, current_case, original_case, final_case):
        next_case = copy.deepcopy(current_case)
        invalid_process_index = []

        # if no anomaly is detected, randomly choose a process from the final case
        if len(self.anomaly_case) == 0:
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
            candidate_index = []
            # indexed according to the final case
            distance_to_anomaly = []
            # calculate each candidate's distance to anomaly
            for i in range(len(current_case)):
                if current_case.param[i].qp_num == 0:
                    candidate_index.append(i)
                    dist = -1
                    current_case.param[i].qp_num = final_case.param[i].qp_num
                    for j in range(len(self.anomaly_case)):
                        if dist == -1:
                            dist = analyzer.case_diff(self.anomaly_case, current_case)
                        else:
                            dist = min(dist, analyzer.case_diff(self.anomaly_case, current_case))
                        current_case.param[i].qp_num = 0
                    distance_to_anomaly.append(dist)
                else:
                    distance_to_anomaly.append(-1)
            assert len(distance_to_anomaly) > 0
            max_dist = max(distance_to_anomaly)
            max_index = distance_to_anomaly.index(max_dist)
            next_case.param[max_index] = copy.deepcopy(final_case.param[max_index])
            return next_case