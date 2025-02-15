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

    def record_case_throughput(self, case: test_case.test_case, throughput: float):
        print(f"anomaly ID: {self.anomaly_id}, throughput: {throughput}")
        self.anomaly_id += 1

    def launch_test(self):
        # in the whole test, iterate for several times
        for i in range(self.config.round_num):
            print(f"start round {i}")
            start_case = self.set_start_case()
            case = start_case
            # todo: judge corner
            while self.analyzer.case_larger(case, self.config.terminus) != True: # if the test case doesn't exceed terminus
                self.driver.start_test(case) # run test and generate test_result_xx files
                time.sleep(10)
                self.driver.stop_test()
                throughput = self.analyzer.calculate_throughput(case)
                print(f"throughput: {throughput}")
                if throughput > 0.2:
                    # todo: record the current case parameters and throughput
                    self.record_case_throughput(case, throughput)
                # debug
                # return
                case = self.set_next_case(case, start_case, self.config.terminus)
                # temp set
                if case == None:
                    break

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
        return start_case

    # set the next case to run according to the current case, the original case, the final case, 
    # and the throughput anomaly
    def set_next_case(self, current_case, original_case, final_case):
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
            return next_case

    # test peak performance of each kind of instance, and record the result in recorder
    # def test_norm(self):
        # todo
