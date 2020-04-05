TARGET = a.out
#OBJS= http3.o
OBJS= 10-at-a-time.o
#CC = gcc
CC = g++
CFLAGS = -c -g --std=c++1z -I/usr/local/opt/curl/include 

LDFLAGS = -L/usr/local/opt/curl/lib -L/usr/local/ssl/lib

# We need -lcurl for the curl stuff
# We need -lsocket and -lnsl when on Solaris
# We need -lssl and -lcrypto when using libcurl with SSL support
# We need -lpthread for the pthread example
#LIBS = -lcurl -lsocket -lnsl -lssl -lcrypto
LIBS = -lcurl 

$(TARGET) : $(OBJS)
	$(CC)  -o $(TARGET) $(OBJS) $(LDFLAGS) $(LIBS)

http3.o : http3.c
	$(CC) $(CFLAGS) $<

10-at-a-time.o : 10-at-a-time.c
	$(CC) $(CFLAGS) $<


