#!/bin/bash

# run as
# 
# $ ./server.out
#
gcc -o server.out web_server.cpp  -lresolv -lpthread

