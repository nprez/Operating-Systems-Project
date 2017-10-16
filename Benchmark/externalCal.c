// File:	externalMerge.c
// Author:	Yujie REN
// Date:	09/23/2017

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <pthread.h>

//#include "../my_pthread_t.h"

#define THREAD_NUM 10
#define RAM_SIZE 160
#define RECORD_NUM 10
#define RECORD_SIZE 1024

pthread_mutex_t   mutex;

pthread_t thread[THREAD_NUM];

int *mem = NULL;

int sum = 0;

int itr = RECORD_SIZE / (RAM_SIZE / THREAD_NUM);

void external_calculate(void* arg) {
	int i = 0, j = 0;
	int n = *((int*) arg);

	char a[3];
	char path[20] = "./record/";

	sprintf(a, "%d", n);
	strcat(path, a);

	int fd = open(path, O_RDONLY);

	for (i = 0; i < itr; ++i) {
		// read 16B from nth record into mem[n]
		read(fd, mem + n*16, 16);
		for (j = 0; j < 4; ++j) {
			pthread_mutex_lock(&mutex);
			sum += mem[n*16 + j];
			pthread_mutex_unlock(&mutex);
		}
	}
	close(fd);
}


int main() {
	int i = 0;
	int counter[THREAD_NUM];

	for (i = 0; i < THREAD_NUM; ++i)
		counter[i] = i;

	mem = (int*)malloc(RAM_SIZE);
	memset(mem, 0, RAM_SIZE);

	pthread_mutex_init(&mutex, NULL);

	for (i = 0; i < THREAD_NUM; ++i)
		pthread_create(&thread[i], NULL, &external_calculate, &counter[i]);

	for (i = 0; i < THREAD_NUM; ++i)
		pthread_join(thread[i], NULL);

	pthread_mutex_destroy(&mutex);

	free(mem);

	return 0;
}
