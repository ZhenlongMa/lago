import os
import time
import re
import atexit
import test_config

class case_driver:

    def __init__(self, test_config: test_config.test_config):
        self.test_config = test_config
        self.test_time = 10 # seconds

    def stop_test(self):
        print("Cleaning perftest processes...")
        machine_list = [self.test_config.servers[0], self.test_config.clients[0]]
        for node in machine_list:
            cmd = f"ssh {self.test_config.user}@{node} 'ps -aux > {self.test_config.object_directory}/tmp.log'"
            print(cmd)
            rtn = os.system(cmd)
            if rtn != 0:
                raise Exception("\033[0;31;40mError for cmd \033[0m")
            proc_id = []
            with open(self.test_config.object_directory + "/tmp.log", "r", encoding ="utf-8") as f:
                for line in f.readlines():
                    line = line.strip()
                    is_match = re.findall(r"(ib_write_bw)|(ib_write_lat)|(ib_read_bw)|(ib_read_lat)", line)
                    if is_match != []:
                        line_list = line.split()
                        pid_num = line_list[1].strip()
                        proc_id.append(pid_num)
                        # kill_cmd = f"ssh {self.test_config.user}@{node} 'kill -9 {pid_num}'"
                        # print(kill_cmd)
                        # os.system(kill_cmd)
            if len(proc_id) != 0:
                kill_cmd = f"ssh {self.test_config.user}@{node} 'kill -9 "
                for i in range(len(proc_id)):
                    kill_cmd += f" {proc_id[i]}"
                kill_cmd += "'"
                print(kill_cmd)
                os.system(kill_cmd)
            os.system("rm -rf " + self.test_config.object_directory + "/tmp.log")
            time.sleep(1)
        print("process cleaned!")

    def generate_command(self, base_cmd, qp_num, msg_sz, port, device, server_ip = None, sharing_mr = 0):
        cmd = (
            f"{base_cmd} -p {port} -d {device} -i 1 -l {self.test_config.wqe_num} -m {self.test_config.mtu} "
            f"-c RC -q {qp_num} -F -s {msg_sz} --run_infinitely"
        )
        if sharing_mr == 0:
            cmd += f" --mr_per_qp"
        if server_ip:
            cmd += f" {server_ip}"
        return cmd

    def execute_commands(self, commands):
        for cmd in commands:
            print(f"\033[0;32;40m{cmd}\033[0m")
            rtn = os.system(cmd)
            if rtn != 0:
                raise Exception(f"\033[0;31;40mError for cmd {cmd}\033[0m")
            time.sleep(1)
    
    def start_test(self, case):
        commands = []
        process_num = len(case.param)
        for i in range(process_num):
            if case.param[i].qp_num == 0:
                continue
            svr_cmd = self.generate_command(self.test_config.test_type, case.param[i].qp_num, \
                                            case.param[i].msg_size, 12331 + i, self.test_config.server_devices[0], None, case.param[i].sharing_mr)
            commands.append(f"ssh {self.test_config.user}@{self.test_config.servers[0]} \
                            'cd {self.test_config.object_directory} && {svr_cmd} > test_result_s{i} &'&")
        for i in range(process_num):
            if case.param[i].qp_num == 0:
                continue
            clt_cmd = self.generate_command(self.test_config.test_type, case.param[i].qp_num, \
                                            case.param[i].msg_size, 12331 + i, self.test_config.client_devices[0], self.test_config.servers[0], case.param[i].sharing_mr)
            commands.append(f"ssh {self.test_config.user}@{self.test_config.clients[0]} \
                            'cd {self.test_config.object_directory} && {clt_cmd} > test_result_c{i} &'&")
        self.execute_commands(commands)

    def test(self, case):
        self.start_test(case)
        time.sleep(self.test_time)
        self.stop_test()
