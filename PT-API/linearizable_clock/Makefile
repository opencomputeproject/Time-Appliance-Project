CC = g++
CFLAGS = -O2 -Wall -std=c++11 -pthread 
LDFLAGS = -lpthread -levent -levent_core -levent_pthreads

all: clockTest clockTest2

clockTest2: clockudp.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) 

clockTest: daemon.o socket.o clock.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ 

%.o: %.cpp
	${CC} $(CFLAGS) -o $@  -c $<

.PHONY: clean
clean:
	-rm -f *.o *.log clockTest clockTest2

format:
	clang-format -i *.cpp *.h 
