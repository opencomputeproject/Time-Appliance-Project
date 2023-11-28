#  Makefile to build testptp
#  Ahmad Byagowi
#
CC = gcc
CFLAGS = -Wall -lrt 

testptp: testptp.c
	$(CC) $(CFLAGS) $^ -o $@ 
install: 
	mv testptp /usr/bin/
clean:
	-rm -f testptp
