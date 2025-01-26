import os
import time
import re
import atexit
import test_config as tc
import iterator as iterator
import case_driver

def main():
    config = tc.test_config()
    config.init_case()
    drv = case_driver.case_driver(config)
    it = iterator.iterator(config)

if __name__ == "__main__":
    main()