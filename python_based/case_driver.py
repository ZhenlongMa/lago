import os
import time
import re
import atexit
import test_config

class case_driver:

    def __init__(self, test_config: test_config.test_config):
        self.test_config = test_config
        self.test_time = 10 # seconds

    def stop_perftest(self, machine_list):
        print("Cleaning perftest processes...")
        for node in machine_list:
            cmd = f"ssh {self.test_config.user}@{node} 'ps -aux > {self.test_config.object_directory}/tmp.log'"
            print(cmd)
            rtn = os.system(cmd)
            if rtn != 0:
                raise Exception("\033[0;31;40mError for cmd \033[0m")
            with open(self.test_config.object_directory + "/tmp.log", "r", encoding ="utf-8") as f:
                for line in f.readlines():
                    line = line.strip()
                    is_match = re.findall(r"(ib_write_bw)|(ib_write_lat)|(ib_read_bw)|(ib_read_lat)", line)
                    if is_match != []:
                        line_list = line.split()
                        pid_num = line_list[1].strip()
                        kill_cmd = "ssh {self.test_config.user}@{node} 'kill -9 {pid_num}'"
                        print(kill_cmd)
                        os.system(kill_cmd)
            os.system("rm -rf " + self.test_config.object_directory + "/tmp.log")
            time.sleep(1)
        print("process cleaned!")

    def generate_command(self, base_cmd, qp_num, msg_sz, port, device, server_ip=None):
        cmd = (
            f"{base_cmd} -p {port} -d {device} -i 1 -l {self.test_config.wqe_num} -m {self.test_config.mtu} "
            f"-c RC -x {self.test_config.gid_index} -q {qp_num} -F -s {msg_sz} --run_infinitely"
        )
        if server_ip:
            cmd += f" {server_ip}"
        return cmd

    def execute_commands(self, commands):
        for cmd in commands:
            print(f"\033[0;32;40m{cmd}\033[0m")
            rtn = os.system(cmd)
            if rtn != 0:
                raise Exception(f"\033[0;31;40mError for cmd {cmd}\033[0m")
            time.sleep(0.1)
    
    def start_test(self):
        commands = []
        process_num = self.test_config.case_param.size()
        for i in range(process_num):
            svr_cmd = self.generate_command(self.test_config.test_type, self.test_config.case_param[i].qp_num, \
                                            self.test_config.case_param[i].msg_sz, 12331 + i, self.test_config.server_devices[0])
            commands.append(f"ssh {self.test_config.user}@{self.test_config.servers[i]} \
                            'cd {self.test_config.object_directory} && {svr_cmd} > test_result_s{i + 1} &'&")
        for i in range(process_num):
            clt_cmd = self.generate_command(self.test_config.test_type, self.test_config.case_param[i].qp_num, \
                                            self.test_config.case_param[i].msg_sz, 12331 + i, self.test_config.client_devices[0], self.test_config.servers[i])
            commands.append(f"ssh {self.test_config.user}@{self.test_config.clients[i]} \
                            'cd {self.test_config.object_directory} && {clt_cmd} > test_result_c{i + 1} &'&")
        self.execute_commands(commands)

    # def norm_test(self, norm_config):


if __name__ == "__main__":
    atexit.register(lambda: case_driver.stop_perftest(SERVERS + CLIENTS))
    main()