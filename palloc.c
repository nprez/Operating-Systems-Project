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
 * Beginning of blocks
 * Block Layout:
 * Metadata:
 * 1 byte for allocated/unallocated
 * 4 bytes for block size
 * Data
 */

//combines 4 chars into an int using bitwise operations
static int fourCharToInt(char a, char b, char c, char d){
	return (a<<24)|(b<<16)|(c<<8)|d;
}

//returns the TID of the given page
static my_pthread_t getPageTid(int pageNum){
	return fourCharToInt(
		memory[pageNum*PAGE_SIZE],
		memory[pageNum*PAGE_SIZE+1],
		memory[pageNum*PAGE_SIZE+2],
		memory[pageNum*PAGE_SIZE+3]
	);
}

//sets the tid metadata of the given page to the requested value
static void setPageTid(int pageNum, my_pthread_t tid){
	memory[pageNum*PAGE_SIZE] = tid>>24;
	memory[pageNum*PAGE_SIZE+1] = (tid<<8)>>24;
	memory[pageNum*PAGE_SIZE+2] = (tid<<16)>>24;
	memory[pageNum*PAGE_SIZE+3] = (tid<<24)>>24;
}

//returns the size of the metadata pointed to by i
static int getBlockSize(int i){
	return fourCharToInt(memory[i+1], memory[i+2], memory[i+3], memory[i+4]);
}

//sets the metadata pointed to by i's size equal to capacity
static void setBlockSize(int i, int capacity){
	memory[i+1] = capacity>>24;
	memory[i+2] = (capacity<<8)>>24;
	memory[i+3] = (capacity<<16)>>24;
	memory[i+4] = (capacity<<24)>>24;
}

//checks if the metadata pointed to by i is marked as allocated
static char isAllocated(int i){
	return memory[i] == 1;
}

//checks if the given page has an unallocated block of size capacity or greater
static char hasSpace(int pageNum, int capacity){
	int i = pageNum*PAGE_SIZE+4;
	if(pageNum>=MEMORY_SIZE/PAGE_SIZE-4)
		i = pageNum*PAGE_SIZE;
	for(i=i; i<(pageNum+1)*PAGE_SIZE; i+=getBlockSize(i)+5){
		if(!isAllocated(i) && getBlockSize(i)>=capacity){
			return 1;
		}
	}
	return 0;
}

//our implementation of malloc
void* myallocate(int capacity, char* file, int line, char threadreq){
	int reset = 0;
	if(!getCritical()){
		setCritical(1);
		reset = 1;
	}

	int i;

	if(!firstTime){
		memset(memory, 0, MEMORY_SIZE);
		//initiate each page with an unallocated block the size of a page
		for(i=0; i<MEMORY_SIZE-(4*PAGE_SIZE); i+=PAGE_SIZE){	//normal pages
			setBlockSize(i+4, PAGE_SIZE-9);
		}
		i = MEMORY_SIZE-(4*PAGE_SIZE);
		setBlockSize(i, (4*PAGE_SIZE)-5);	//shared pages
		firstTime = 1;
	}

	if(capacity<0){
		fprintf(stderr, "Error on malloc in file: %s,  on line %d. Cannot allocate a negative amount of memory\n", file, line);
		return NULL;
	}

	my_pthread_t curr = getCurrentTid();
	int temp;
	for(i=0; i<MEMORY_SIZE/PAGE_SIZE-4; i++){	//try to find an open unshared page
		temp = getPageTid(i);
		if(temp == 0 || (temp == curr && hasSpace(i, capacity))){
			break;
		}
	}
	if(i==MEMORY_SIZE/PAGE_SIZE-4){	//out of non shared pages
		return NULL;
	}
	if(curr != temp)	//unallocated page, give it our tid
		setPageTid(i, curr);

	//allocate within page i
	temp = i;
	for(i=i*PAGE_SIZE+4; i<(temp+1)*PAGE_SIZE; i+=getBlockSize(i)+5){
		if(!isAllocated(i) && getBlockSize(i)>=capacity){
			memory[i] = 1;	//mark allocated
			int oldSize = getBlockSize(i);
			if(oldSize-capacity>=5){	//enough room to split the block without complications
				setBlockSize(i, capacity);
				//split block
				memory[i+5+capacity] = 0;
				setBlockSize(i+5+capacity, oldSize-(capacity+5));
				int nextI = i+5+oldSize;
				if(nextI%PAGE_SIZE != 0 && !isAllocated(nextI)){	//check if next block free
					//combine free blocks
					int currSize = getBlockSize(i+5+capacity);
					int nextSize = getBlockSize(nextI);
					setBlockSize(i+5+capacity, currSize+5+nextSize);
				}
			}
			//otherwise, leave the user oldSize capacity to avoid complications
			break;
		}
	}

	if(reset)
		setCritical(0);

	return &memory[i+5];
}

//our implementation of free
void mydeallocate(void* toBeFreed, char* file, int line, char threadreq){
	//check if pointer to be freed is within the memory block
	int difference = toBeFreed - ((void*)memory);
	if(difference < 0 || difference > MEMORY_SIZE){
		fprintf(stderr, "Error on free in file: %s,  on line %d. Not within memory block\n", file, line);
		return;
	}

	//check if the thread calling free is allowed to access this page
	int pageItsIn = difference / PAGE_SIZE;
	int i;
	//any thread can free shared memory
	char shared = 0;
	if((MEMORY_SIZE / PAGE_SIZE) - pageItsIn >= 4){
		if(getCurrentTid() != getPageTid(pageItsIn)){
			fprintf(stderr, "Error on free in file: %s, on line %d. Page blocked from thread.\n", file, line);
			return;
		}
	}
	else
		shared = 1;

	i = pageItsIn * PAGE_SIZE + 4;
	if(shared)
		i = MEMORY_SIZE-(4*PAGE_SIZE);

	int allocatedBlocks = 0;
	char found = 0;
	int bound = (pageItsIn + 1)*PAGE_SIZE;
	if(shared)
		bound = MEMORY_SIZE;
	//looking at beginning of allocated blocks within the page is the location being pointed to
	for(i = i; i < bound; i += getBlockSize(i)+5){
		if(isAllocated(i))
			allocatedBlocks++;

		if(&memory[i+5] == toBeFreed){
			if(!isAllocated(i)){
				fprintf(stderr, "Error on free in file: %s, on line %d. Address already free.\n", file, line);
				return;
			}
			memory[i] = 0;

			//adding new free with next free block together if that exists
			int capacity = getBlockSize(i);
			if((i+capacity+5)%PAGE_SIZE != 0 && !isAllocated(i+capacity+5)){
				capacity += getBlockSize(i+capacity+5) + 5;
				setBlockSize(i,capacity);
			}
			//adding previous free block together with current free block if it exists
			int oldI = i;
			int prevBlockSize = -1;
			i = pageItsIn*PAGE_SIZE;
			if(!shared)
				i+=4;
			for(i = i; i < oldI; i += getBlockSize(i)+5)
				prevBlockSize = getBlockSize(i);
			if(prevBlockSize != -1){
				i -= (prevBlockSize+5);
				if(memory[i] == 0){
					int currSize = getBlockSize(i);
					int nextSize = getBlockSize(i + currSize + 5);
					setBlockSize(i, currSize + 5 + nextSize);
				}
			}
			found = 1;
		}
	}

	//free the page
	if(!shared && (allocatedBlocks == 1)){
		setPageTid(pageItsIn, 0);
	}

	if(!found)
		fprintf(stderr, "Error on free in file: %s, on line %d. Pointer not reference to beginning of allocated block.\n", file, line);
}

//allocates a chunk of shared memory of the given size
void* shalloc(size_t size){
	int i = MEMORY_SIZE-(4*PAGE_SIZE);
	for(i=i; i<MEMORY_SIZE; i+=getBlockSize(i)+5){
		if(!isAllocated(i) && getBlockSize(i)>=size){
			//allocate the block
			memory[i] = 1;

			int oldSize = getBlockSize(i);
			if(oldSize-size>=5){	//enough room to split the block without complications
				setBlockSize(i, size);
				//split block
				memory[i+5+size] = 0;
				setBlockSize(i+5+size, oldSize-(size+5));
				int nextI = i+5+oldSize;
				if(!isAllocated(nextI)){	//check if next block free
					//combine free blocks
					setBlockSize(i+5+size, getBlockSize(i+5+size)+5+getBlockSize(nextI));
				}
			}
			//otherwise, leave the user oldSize size to avoid complications
			return &memory[i+5];
		}
	}
	return NULL;
}
