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
	int temp = i;
	for(i=i*PAGE_SIZE+4; i<(temp+1)*PAGE_SIZE; i+=getBlockSize(i)+5){
		if(!isAllocated && getBlockSize(i)>=capacity){
			//mark allocated
			memory[i] = 1;
			//set size
			int oldSize = getBlockSize(i);
			setBlockSize(i, capacity);
			if(oldSize>capacity){
				int remainder = oldSize-capacity;
				if(remainder<5){
					//get user the extra space
					setBlockSize(i, );
				}
				else{
					//split block
					//check if next chunk free
					//combine if next chunk free
				}
			}
			break;
		}
	}
	return &memory[i+5]
}
void mydeallocate(void* toBeFreed, char* file, int line, char threadreq){

  //check if pointer to be freed is within the memory block
  int difference = toBeFreed - memory;
  if(difference < 0 || difference > MEMORY_SIZE){
      fprintf(stderr, "Error on free in file: %s,  on line %d. Not within memory block\n", file, line);
      return;
    }

  //check if the thread calling free is allowed to access this page
  int pageItsIn = toBeFree / PAGE_SIZE;
  int i;
  if(getCurrentTid() != getPageTid(pageItsIn)){
      fprintf(stderr, "Error on free in file: %s, on line %d. Page blocked from thread.\n", file, line);
      return;
    }

  //looking at beginning of allocated chunks within the page is the location being pointed to
  for(i = (pageItsIn * PAGE_SIZE)+4; i < (pageItsIn + 1)*PAGE_SIZE; i += getBlockSize(i)+5){
      if(memory[i+5] == toBeFreed){
	  memory[i] = 0;

	  //adding new free with next free block together if that exists
	  int capacity = fourCharToInt(memory[i+1], memory[i+2], memory[1+3], memory[i+4]);
	  if(memory[i+capacity+5] == 0){
	      capacity += fourCharToInt(memory[i+capacity+6], memory[i+capacity+7], memory[i+capacity+8], memory[i+capacity+9]);
	      setBlockSize(i,capacity);
	    }
	  return;
	}
    }

      fprintf(stderr, "Error on free in file: %s, on line %d. Pointer not reference to beginning of allocated chunk.\n", file, line);
      return;
}
