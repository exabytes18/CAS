CC=gcc
OPTS=-O3
LIBS=-lpthread

all: cas

cas: cas.c
	$(CC) $(OPTS) $(LIBS) cas.c -o cas

clean:
	-rm -rf cas

