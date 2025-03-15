import os
import time
import re
import atexit
import test_config as tc
import iterator as iterator
import case_driver
import analyzer

def main():
    # remove result files
    os.system("rm test_result_*")
    config = tc.test_config("28")
    # config.init_case()
    performance_analyzer = analyzer.analyzer(config)
    drv = case_driver.case_driver(config)
    it = iterator.iterator(config, drv, performance_analyzer)
    print("begin test!")
    atexit.register(drv.stop_test)
    it.launch_test()
    print("end test!")

if __name__ == "__main__":
    main()
