
# TODO

all: blambd.out blog.out

blambd.out: boost_lambda.cpp
	g++ -std=c++1z boost_lambda.cpp -o blambd.out -I $(BOOST_ROOT) -L $(BOOST_ROOT_LIB)

blog.out: boost_log.cpp
	#gcc -std=c++1z boost_log.cpp -o blog.out -I $(BOOST_ROOT) -L $(BOOST_ROOT_LIB) -lboost_log -lboost_timer -lboost_system -lboost_thread
	# worked on linux:
	#g++ -std=c++1z boost_log.cpp -o blog.out -I $(BOOST_ROOT) -L $(BOOST_ROOT_LIB) -lboost_log -lboost_log_setup -lboost_timer -lboost_system -lboost_thread -pthread
	g++ -std=c++1z boost_log.cpp -o blog.out -I $(BOOST_ROOT) -L $(BOOST_ROOT_LIB) $(BOOST_ROOT_LIB)/libboost_log.a -lboost_log_setup -lboost_timer -lboost_system -lboost_thread -pthread

# g++ -static -std=c++1z boost_log.cpp -o blog.out -I$(BOOST_ROOT) -L$(BOOST_ROOT_LIB) -pthread -lboost_system -lboost_log_setup -lboost_log -lboost_date_time -lboost_thread -lboost_filesystem
# -lboost_log -lboost_log_setup -lboost_timer -lboost_system -lboost_thread -pthread
#-lstdc++

clean:
	rm *.out

test:
	echo 1 2 3 | ./blambd.out
	./blog.out

    
