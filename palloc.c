#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include "my_pthread_t.h"

#define malloc(x) myallocate(x,__FILE__,__LINE__,THREADREQ)
#define free(x) mydeallocate(x,__FILE__,__LINE__,THREADREQ)
#define MEMORY_SIZE 1048576
#define THREADREQ 0
#define LIBRARYREQ 1
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

static char memory[MEMORY_SIZE];

void* myallocate(int capacity, char* file, int line, char threadreq)
{

}
void mydeallocate(int capacity, char* file, int line, char threadreq)
{

}
