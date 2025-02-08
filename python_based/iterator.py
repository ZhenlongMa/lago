import case_driver
import test_config
import analyzer
import test_case
import copy
import random

class iterator:
    def __init__(self, config: test_config, driver: case_driver, analyzer: analyzer.analyzer):
        self.config = config
        self.driver = driver
        self.reach_destination = False
        self.analyzer = analyzer

    def launch_test(self):
        # in the whole test, iterate for several times
        for i in range(self.config.round_num):
            start_case = self.set_start_case()
            case = start_case
            while self.analyzer.diff_case(case, self.config.terminus) != True: # if the test case doesn't exceed terminus
                self.driver.start_test() # run test and generate test_result_xx files
                self.analyzer.calculate_throughput(case)
                case = self.set_next_case(case, start_case, self.config.terminus)

    def set_start_case(self):
        start_case = test_case.test_case()
        # start_process_num = 1

        # to do: generate the start case
        for i in range(self.config.terminus.process_num):
            # param = start_case.process_param(1, "RC", "WRITE", 64, False)
            param = start_case.process_param(self.config.terminus.case[i].service_type, self.config.terminus.case[i].op)
            start_case.case.append(param)

        # temp setting: copy the terminus qp num
        start_case.case[0].qp_num = self.config.terminus.case[0].qp_num
        return start_case

    # set the next case to run according to the current case, the original case, the final case, 
    # and the throughput anomaly
    def set_next_case(self, current_case, original_case, final_case):
        next_case = copy.deepcopy(current_case)
        invalid_process_index = []
        for i in range(current_case.case.size()):
            if current_case.case[i].qp_num == 0:
                invalid_process_index.append(i)
        random_process = random.choice(invalid_process_index)
        next_case.case[random_process] = copy.deepcopy(final_case.case[random_process])
        return next_case

    # test peak performance of each kind of instance, and record the result in recorder
    # def test_norm(self):
        # todo