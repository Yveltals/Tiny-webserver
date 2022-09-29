DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2
endif

server: main.cpp ./server/webserver.cpp ./server/epoller.cpp ./http/httpconn.cpp ./pool/sqlconnpool.cpp
	g++ -o webserver $^ -lpthread -lmysqlclient

clean:
	rm  -r webserver
