import case_driver
import test_config
import analyzer
import test_case

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
                self.analyzer.calculate_throughput()
                case = self.set_next_case(case, start_case, self.config.terminus)

    def set_start_case():
        start_case = test_case.test_case()
        start_process_num = 1
        # to do: generate the start case
        return start_case

    # set the next case to run according to the current case, the original case, the final case, 
    # and the throughput anomaly
    def set_next_case(self, current_case, original_case, final_case):
        next_case = test_case.test_case()
        # todo: generate the next case
        return next_case

    # test peak performance of each kind of instance, and record the result in recorder
    def test_norm(self):
        # todo