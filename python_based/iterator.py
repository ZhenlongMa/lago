import case_driver
import test_config
import analyzer

class iterator:
    def __init__(self, config: test_config, driver: case_driver, analyzer: analyzer.analyzer):
        self.config = config
        self.driver = driver
        self.reach_destination = False
        self.analyzer = analyzer
    
    # set test destination of each iteration
    def set_terminus(self):
        # todo
        self.config.generate_terminus()

    def launch_test(self):
        for i in range(self.config.round_num):
            self.set_start_case()
            self.set_terminus()
            while self.reach_destination() != True:
                self.driver.start_test() # run test and generate test_result_xx files
                self.analyzer.calculate_throughput()
                self.set_next_case()

    def set_start_case():
        # todo

    def set_next_case(self):
        # todo

    def reach_destination(self):
        # todo

    # test peak performance of each kind of instance, and record the result in recorder
    def test_norm(self):
        # todo