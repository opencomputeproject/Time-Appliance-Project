CC = g++
CFLAGS = -O2 -Wall -std=c++17 -pthread 
LDFLAGS = -lpthread -lrt -levent -levent_core -levent_pthreads

all: ptptool

ptptool: ptptool.o
	$(CC) $(LDFLAGS) -o $@ $^ 

%.o: %.cpp
	${CC} $(CFLAGS) -o $@  -c $<

.PHONY: clean
clean:
	-rm -f *.o *.log ptptool

format:
	clang-format -i *.cpp 
