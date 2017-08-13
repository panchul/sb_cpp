#!/bin/bash

gcc -o client.out daytimeclient.cpp  -lresolv -lpthread
gcc -o server.out daytimeserver.cpp  -lresolv -lpthread

