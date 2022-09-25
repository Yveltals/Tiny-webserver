DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2
endif

server: webserver.cpp ./utils/utils.cpp ./http/http_conn.cpp ./pool/sqlconnpool.cpp
	g++ -o server $^ -lpthread -lmysqlclient

clean:
	rm  -r server
