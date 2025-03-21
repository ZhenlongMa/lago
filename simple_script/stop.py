def stop_perftest(node_list):
    for node in node_list:
        proc_id = []
        cmd = "ssh root@" + node + " 'ps -aux > " + OBJ_DIR + "/tmp.log'"
        print(cmd)
        rtn = os.system(cmd)
        if rtn != 0:
            raise Exception("\033[0;31;40mError for cmd \033[0m")
        with open(OBJ_DIR + "/tmp.log", "r", encoding ="utf-8") as f:
            for line in f.readlines():
                line = line.strip()
                is_match = re.findall(r"(ib_write_bw)|(ib_write_lat)", line)
                if is_match != []:
                    line_list = line.split()
                    pid_num = line_list[1].strip()
                    proc_id.append(pid_num)
                    kill_cmd = "ssh root@" + node + " 'kill -9 " + pid_num + "'"
                    print(kill_cmd)
                    os.system(kill_cmd)
        os.system("rm -rf " + OBJ_DIR + "/tmp.log")
        time.sleep(1)