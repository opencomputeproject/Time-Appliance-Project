CC = g++
CFLAGS = -O2 -Wall -std=c++17 -pthread 
LDFLAGS = -lpthread 

all: diffphc

diffphc: diffphc.o
	$(CC) $(LDFLAGS) -o $@ $^ 

%.o: %.cpp
	${CC} $(CFLAGS) -o $@  -c $<

.PHONY: clean
clean:
	-rm -f *.o *.log diffphc

format:
	clang-format -i *.cpp 
