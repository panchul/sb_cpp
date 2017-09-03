#!/bin/bash

# run as
# 
# $ ./client.out
#
gcc -o client.out daytimeclient.cpp  -lresolv -lpthread

# run as
# 
# $ ./server.out
#
gcc -o server.out daytimeserver.cpp  -lresolv -lpthread

