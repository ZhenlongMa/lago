import sys

if sys.argv[1] == "server":
    os.system("ib_write_bw -p 1234 -d mlx5_0 -i 1 -l 100 -m 1024 -c RC -x 0 -q 4 -s 64 --run_infinitely
