import os
import time
import re
import atexit
import test_config as tc
import iterator as iterator
import case_driver

def main():
    config = tc.test_config("28")
    config.init_case()
    drv = case_driver.case_driver(config)
    it = iterator.iterator(config, drv)
    print("begin test!")
    it.launch_test()
    print("end test!")

if __name__ == "__main__":
    main()