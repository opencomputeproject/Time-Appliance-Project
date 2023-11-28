#  Makefile to build testptp
#  Ahmad Byagowi
#
CC = gcc
CFLAGS = -Wall -lrt 

testptp: testptp.c
	$(CC) $(CFLAGS) $^ -o $@ 
install: 
	mv testptp /root/bin/
clean:
	-rm -f testptp
