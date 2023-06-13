#  Makefile to build tangle and timesrv programs
#
#
CC = cc
CFLAGS = -O3 -Wall -pthread 
LDFLAGS = -lpthread 

all: tangle timesrv

tangle: tangle.o
	$(CC) $(LDFLAGS) -o $@ $^ 
timesrv: timesrv.c
	$(CC) $(LDFLAGS) -o $@ $^ 
%.o: %.c
	${CC} $(CFLAGS) -o $@  -c $<

.PHONY: clean
format:
	clang-format -i *.c 
clean:
	-rm -f *.o *.log tangle timesrv
