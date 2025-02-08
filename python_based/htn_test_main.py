import os
import time
import re
import atexit
import test_config as tc
import iterator as iterator
import case_driver
import analyzer

def main():
    config = tc.test_config("28")
    config.init_case()
    analyzer = analyzer.analyzer(config)
    drv = case_driver.case_driver(config)
    it = iterator.iterator(config, drv, analyzer)
    print("begin test!")
    atexit.register(lambda: case_driver.stop_perftest(SERVERS + CLIENTS))
    it.launch_test()
    print("end test!")

if __name__ == "__main__":
    main()