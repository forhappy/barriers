all:bench

CFLAGS=-Wall -g -std=gnu99

bench:bench.o pthread-barrier.o futex-barrier.o ticket-barrier.o fast-barrier.o
	gcc -o $@ $^ -lpthread

bench.o:bench.c
	gcc $(CFLAGS) -o $@ -c $^

pthread-barrier.o:pthread-barrier.c
	gcc $(CFLAGS) -o $@ -c $^

futex-barrier.o:futex-barrier.c
	gcc $(CFLAGS) -o $@ -c $^

ticket-barrier.o:ticket-barrier.c
	gcc $(CFLAGS) -o $@ -c $^

fast-barrier.o:fast-barrier.c
	gcc $(CFLAGS) -o $@ -c $^

.PHONY:clean

clean:
	rm bench bench.o pthread-barrier.o futex-barrier.o ticket-barrier.o fast-barrier.o
