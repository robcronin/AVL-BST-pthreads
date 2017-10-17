CC = gcc
CFLAGS = -W -Wall
LDLIBS = -lm

objects = serial.o pthreads.o
executables = serial.out pthreads.out

.PHONY: all clean stest ptest

all: serial.out pthreads.out

serial.out: serial.o
	$(CC) $(CFLAGS) serial.o -o $@ $(LDLIBS)

pthreads.out: pthreads.o
	$(CC)  $(CFLAGS) pthreads.o -o $@ $(LDLIBS) -lpthread


clean :
	$(RM) $(objects) $(executables)

stest: serial.out
	./$^

ptest: pthreads.out
	./$^
