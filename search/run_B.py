# Run this Python script in 10.18.133.28 (node28)
import os
import time
import re
import atexit

case_file = "../collie_based/test_case_demo"
EINGINE_PATH = "/work/mazhenlong/rnic_test/collie_based/test_engine"
user = "root"
node_list = ["10.5.200.186", "10.5.200.187"]
SERVER_IP = "192.168.0.23"
CLIENT_IP = "92.168.0.25"
dev = "mlx5_0"
OBJ_DIR = "/home/mazhenl/shared/rnic_test/search"

def start_test():
    # launch server
    server_parameter = "--server --dev=mlx5_0"
    cmd = EINGINE_PATH + " " + server_parameter + " > " + OBJ_DIR + "/server_log &"
    print(cmd)
    rtn = os.system(cmd)
    if (rtn != 0):
        raise Exception("Error for cmd!")
    # launch client
    client_parameter = "--connect=10.5.200.186 --dev=mlx5_0"
    cmd = "ssh " + user + "@" + CLIENT_IP + " '" + EINGINE_PATH + " " + client_parameter + " > " + OBJ_DIR + "/client_log &'&" 
    print(cmd)
    rtn = os.system(cmd)
    if (rtn != 0):
        raise Exception("Error for cmd!")

def stop():
    keyword = "test_engine"
    # Stop local process using netstat
    cmd = "netstat -t -p > " + OBJ_DIR + "/tmp.log"
    print(cmd)
    rtn = os.system(cmd)
    if rtn != 0:
        raise Exception("Error for cmd!")
    with open(OBJ_DIR + "/tmp.log", "r") as f:
        for line in f.readlines():
            line = line.strip()
            str_list = line.split(' ')
            res = re.findall(r"(.+?)/"+keyword, str_list[-1])
            print(res)
            if res != []:
                cmd = "kill -9 " + res[0]
                os.system(cmd)
    # Stop remote process using ps
    cmd = "ssh mazhenl@" + CLIENT_IP + " 'ps -aux > " + OBJ_DIR +"/tmp.log'"
    print(cmd)
    rtn = os.system(cmd)
    if (rtn != 0):
        raise Exception("Error for cmd!")
    with open(OBJ_DIR + "/tmp.log", "r") as f:
        for line in f.readlines():
            line = line.strip()
            is_match = re.findall(keyword, line)
            if is_match != []:
                line_list = line.split()
                pid_num = line_list[1].strip()
                kill_cmd = "ssh " + user + "@" + CLIENT_IP + " 'kill -9 " + pid_num + "'"
                print(kill_cmd)
                os.system(kill_cmd)
    os.system("rm -rf " + OBJ_DIR + "/tmp.log")
    time.sleep(3)
    print("process cleaned!")

def main():
    with open(case_file, "w") as f:
        f.write("2 2 2 2 2 2 2")
    print("finish writing case file!")
    start_test()
    print("start test!")
    time.sleep(5)
    stop()
    print("finish test!")
    
if __name__ == "__main__":
    atexit.register(stop)
    main()