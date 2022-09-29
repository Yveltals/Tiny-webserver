CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = webserver
OBJS = ./pool/*.cpp ./timer/*.cpp ./http/*.cpp ./server/*.cpp ./main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf $(TARGET)