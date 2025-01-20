# Run this Python script in 10.18.133.28 (node28)
import os
import time
import re
import atexit

case_file = "../collie_based/test_case_demo"
EINGINE_PATH = "/work/mazhenlong/rnic_test/collie_based/test_engine"
user = "root"
SERVER_IP = "192.168.0.23"
CLIENT_IP = "192.168.0.25"
dev = "mlx5_0"
OBJ_DIR = "/work/mazhenlong/rnic_test/search"

def start_test():
    if 1:
        server_parameter = "--server --dev=mlx5_0"
        client_parameter = "--connect_ip=" + SERVER_IP + " --dev=mlx5_0"
        # launch server
        cmd = "ssh " + user + "@" + SERVER_IP + " ' cd " + OBJ_DIR + " && " + EINGINE_PATH + " " + server_parameter + " > " + OBJ_DIR + "/server_log 2>&1 &'&"
        print(cmd)
        rtn = os.system(cmd)
        if (rtn != 0):
            raise Exception("Error for cmd!")
        print("start server!")
        # launch client
        cmd = "ssh " + user + "@" + CLIENT_IP + " ' cd " + OBJ_DIR + " && " + EINGINE_PATH + " " + client_parameter + " > " + OBJ_DIR + "/client_log 2>&1 &'&" 
        print(cmd)
        rtn = os.system(cmd)
        if (rtn != 0):
            raise Exception("Error for cmd!")
        print("start client")
    else:
        cmd = "ssh " + user + "@" + SERVER_IP + " '" + "echo $PATH" + " > " + OBJ_DIR + "/server_log &'&"
        print(cmd)
        rtn = os.system(cmd)

def stop():
    keyword = "test_engine"
    # Stop remote process using ps
    cmd = "ssh " + user + "@" + SERVER_IP + " 'ps -aux > " + OBJ_DIR +"/tmp.log'"
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
                kill_cmd = "ssh " + user + "@" + SERVER_IP + " 'kill -9 " + pid_num + "'"
                print(kill_cmd)
                os.system(kill_cmd)
    os.system("rm -rf " + OBJ_DIR + "/tmp.log")
    time.sleep(3)
    print("server process cleaned!")
    
    cmd = "ssh " + user + "@" + CLIENT_IP + " 'ps -aux > " + OBJ_DIR +"/tmp.log'"
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
    print("client process cleaned!")

def compile():
    os.system("cd ../collie_based && rm test_engine && make")
    print("----------------compile finished--------------------")

def main():
    with open(case_file, "w") as f:
        f.write("2 2 2 2 2 2 2")
    print("finish writing case file!")
    compile()
    start_test()
    print("start test!")
    time.sleep(10)
    stop()
    print("finish test!")
    
if __name__ == "__main__":
    atexit.register(stop)
    main()