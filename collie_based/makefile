# MIT License

# Copyright (c) 2021 ByteDance Inc. All rights reserved.
# Copyright (c) 2021 Duke University. All rights reserved.

# See LICENSE for license information


# make clean; make for non-GDR version
# make clean; GDR=1 make for GDR version
name = collie_engine
objects = htn_main.o htn_helper.o htn_endpoint.o htn_memory.o htn_context.o
headers = htn_helper.hh htn_context.hh htn_endpoint.hh htn_memory.hh
CC = g++

CFLAGS = -O3
LDFLAGS = -libverbs -lmlx5 -lglog -lpthread -lgflags

$(name) : $(objects)
	g++ -o $(name) $(objects) $(LDFLAGS)
	rm -rf $(objects)

$(objects) : %.o : %.cc $(headers)
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY : clean
clean:
	rm $(name)  $(objects) collie_engine_debug
