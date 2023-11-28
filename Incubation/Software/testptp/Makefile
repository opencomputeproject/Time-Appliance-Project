#  Makefile to build testptp
#  Ahmad Byagowi
#
GCC = gcc
CFLAGS = -Wall -lrt 

testptp: testptp.c
	$(GCC) $(CFLAGS) $^ -o $@ 
clean:
	-rm -f testptp
