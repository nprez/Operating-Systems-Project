CC = gcc
CFLAGS = -g -w

all:: parallelCal vectorMultiply externalCal

parallelCal: 
	$(CC) $(CFLAGS) -pthread -o parallelCal parallelCal.c -L../ -lmy_pthread

vectorMultiply: 
	$(CC) $(CFLAGS) -pthread -o vectorMultiply vectorMultiply.c -L../ -lmy_pthread

externalCal: 
	$(CC) $(CFLAGS) -pthread -o externalCal externalCal.c -L../ -lmy_pthread
	
pthread_tester: 
	$(CC) $(CFLAGS) -pthread -o pthread_tester pthread_tester.c -L../ -lmy_pthread

clean:
	rm -rf parallelCal vectorMultiply externalCal pthread_tester*.o