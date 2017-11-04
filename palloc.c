#include "my_pthread_t.h"
#include <string.h>

#define malloc(x) myallocate(x,__FILE__,__LINE__,THREADREQ)
#define free(x) mydeallocate(x,__FILE__,__LINE__,THREADREQ)
#define MEMORY_SIZE 1048576
#define THREADREQ 0
#define LIBRARYREQ 1
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

static char memory[MEMORY_SIZE];
static char firstTime = 0;

/*
 * Page Layout:
 * 4 bytes for TID
 * Beginning of chunks
 * Chunk Layout:
 * 1 byte for allocated/unallocated
 * 4 bytes for chunk size
 */

int fourCharToInt(char a, char b, char c, char d){
	return (a<<24)|(b<<16)|(c<<8)|d;
}

my_pthread_t getCurrentTid(int pageNum){
	return fourCharToInt(memory[pageNum*PAGE_SIZE],
			memory[pageNum*PAGE_SIZE+1],
			memory[pageNum*PAGE_SIZE+2],
			memory[pageNum*PAGE_SIZE+3]);
}

int getBlockSize(int i){
	return fourCharToInt(memory[i+1], memory[i+2], memory[i+3], memory[i+4]);
}

char isAllocated(int i){
	return memory[i] == 1;
}

char hasSpace(int pageNum, int capacity){
	int i;
	for(i=(pageNum+1)*PAGE_SIZE+4; i<(pageNum+2)*PAGE_SIZE; i+=getBlockSize(i)+5){
		if(getBlockSize(i)+5>=capacity && !isAllocated(i)){
			return 1;
		}
	}
	return 0;
}

void* myallocate(int capacity, char* file, int line, char threadreq){
	if(!firstTime){
		//fix this later
		memset(memory, 0, MEMORY_SIZE);
		firstTime = 1;
	}
	my_pthread_t curr = getCurrentTid();
	int i;
	for(i=0; i<MEMORY_SIZE/PAGE_SIZE; i++){
		int temp = getPageTid(i);
		if(temp == 0 || (temp == curr && hasSpace(i, capacity))){
			break;
		}
	}
	if(i==MEMORY_SIZE/PAGE_SIZE){
		//out of pages
		return NULL
	}
	//allocate within i
}
void mydeallocate(int capacity, char* file, int line, char threadreq){

}
