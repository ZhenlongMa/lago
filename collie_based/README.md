input format for server:

`./test_engine --server --dev=mlx5_0 --gid=3 --mtu=3 --buf_num=4 --mr_num=4 --buf_size=65536`

input format for client:

`./collie_engine --connect_ip=192.168.0.1 --dev=mlx5_0 --gid=3 --qp_type=2 --mtu=3 --qp_num=1 --buf_num=4 --mr_num=4 --mr_size=65536`
