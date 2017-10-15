#!/bin/bash

# run as
# 
# $ ./server.out
#
gcc -o server.out web_server.cpp  -lresolv -lpthread

gcc -o pic_server.out pic_server.cpp  -lresolv -lpthread
gcc -o pic_client.out pic_client.cpp  -lresolv -lpthread

gcc -o json_server.out json_server.cpp  -lresolv -lpthread
