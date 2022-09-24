DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2
endif

server: webserver.cpp utils.cpp http_conn.cpp
	g++ -o server $^ -lpthread

clean:
	rm  -r server
