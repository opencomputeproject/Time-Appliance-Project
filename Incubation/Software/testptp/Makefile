#  Makefile to build testptp
#  Ahmad Byagowi
#
CC = gcc
CFLAGS = -Wall -lrt 

testptp: testptp.c
	$(CC) $(CFLAGS) $^ -o $@ 
clean:
	-rm -f testptp
