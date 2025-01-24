import os
import time
import re
import atexit

class case_driver:

    def __init__(self, test_config):
        self.test_config = test_config

    def stop_perftest(self, machine_list):
        print("Cleaning perftest processes...")
        for node in machine_list:
            cmd = f"ssh {self.test_config.USER}@{node} 'ps -aux > {self.test_config.OBJ_DIR}/tmp.log'"
            print(cmd)
            rtn = os.system(cmd)
            if rtn != 0:
                raise Exception("\033[0;31;40mError for cmd \033[0m")
            with open(self.test_config.OBJ_DIR + "/tmp.log", "r", encoding ="utf-8") as f:
                for line in f.readlines():
                    line = line.strip()
                    is_match = re.findall(r"(ib_write_bw)|(ib_write_lat)|(ib_read_bw)|(ib_read_lat)", line)
                    if is_match != []:
                        line_list = line.split()
                        pid_num = line_list[1].strip()
                        kill_cmd = "ssh {USER}@{node} 'kill -9 {pid_num}'"
                        print(kill_cmd)
                        os.system(kill_cmd)
            os.system("rm -rf " + self.test_config.OBJ_DIR + "/tmp.log")
            time.sleep(3)
        print("process cleaned!")

if __name__ == "__main__":
    atexit.register(lambda: stop_perftest(SERVERS + CLIENTS))
    main()