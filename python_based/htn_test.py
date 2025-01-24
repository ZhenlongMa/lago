import os
import time
import re
import atexit
import test_config as tc
import decider
import case_driver

def main():
    config = tc.test_config()
    config.init_case()
    drv = case_driver.case_driver()
    dc = decider.decider()

if __name__ == "__main__":
    main()