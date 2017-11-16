// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name: Nicholas Prezioso, Benjamin Cahnbley, and Marcella Alvarez
// username of iLab: njp107, bc499, and ma1143
// iLab Server: prototype

#include "my_pthread_t.h"


static char* memory;
static char* swapMemory;
static int fd;
static char firstTime = 0;

//replace this with a new byte in beginning of page that is binary for 0 = does not contain a thread, 1 = contrains a thread. Remove all instances of containsThread
 
/* Page Layout:
 * 1 byte for allocated/unallocated
 * 4 bytes for TID
 * Beginning of blocks
 * Block Layout:
 * Metadata:
 * 1 byte for allocated/unallocated
 * 4 bytes for block size
 * Data
 */

/* Swap File Page Layout:
 * 1 byte for allocated/unallocated
 * 4 bytes for TID
 * 4 bytes for location in real memory
 * Beginning of blocks
 * Block Layout:
 * Metadata:
 * 1 byte for allocated/unallocated
 * 4 bytes for block size
 * Data
 */

static int thread_inited = 0;
/*Q Max for Threads*/
static queue* queue1;
static queue* queue2;
static queue* queue3;
static node_t* deadQueue;
/*in scheduler or allocating memory*/
static int __CRITICAL__ = 0;

static my_pthread* current_thread = NULL;


void updateMemoryProtections(){
	if(mprotect(memory, MEMORY_SIZE, PROT_READ | PROT_WRITE)){
		perror("Couldn't use mprotect");	
	}
	int i;
	for(i=0; i<(MEMORY_SIZE/PAGE_SIZE)-4; i++){
		my_pthread_t curr = -1;
		if(current_thread != NULL)
			curr = current_thread->tid;
		if(memory[i*PAGE_SIZE]!=1 || getPageTid(i) != curr){
			mprotect(&memory[i*PAGE_SIZE], PAGE_SIZE, PROT_NONE);
		}
	}
}

void setupMemory(){
	int i;
	memory = (char*)memalign(PAGE_SIZE, MEMORY_SIZE);
	memset(memory, 0, MEMORY_SIZE);
	mprotect(memory, MEMORY_SIZE, PROT_READ | PROT_WRITE);
	//initiate each page with an unallocated block the size of a page
	for(i=0; i<MEMORY_SIZE-(4*PAGE_SIZE); i+=PAGE_SIZE){	//normal pages
		setBlockSize(i+5, PAGE_SIZE-10);
	}		
	i = MEMORY_SIZE-(4*PAGE_SIZE);
	setBlockSize(i, (4*PAGE_SIZE)-5);	//shared pages
	fd = open("swapfile", O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
	lseek(fd, (MEMORY_SIZE*2)-1, SEEK_SET);
	write(fd, "", 1);
	swapMemory = mmap(0, MEMORY_SIZE*2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = handler;

	if (sigaction(SIGSEGV, &sa, NULL) == -1){
		printf("Fatal error setting up signal handler\n");
		exit(EXIT_FAILURE);	//explode!
	}

	firstTime = 1;
}

void enqueue(my_pthread* t){
	if(t!=NULL){
		if(t->status == THREAD_DYING){
			__CRITICAL__ = 1;
			node_t* newNode = malloc(sizeof(node_t));
			__CRITICAL__ = 0;
			newNode->tid = t->tid;
			newNode->next = NULL;
			newNode->ret = t->ret;
			if(deadQueue == NULL)
				deadQueue = newNode;
			else{
				node_t* ptr = deadQueue;
				while(ptr->next != NULL)
					ptr = ptr->next;
				ptr->next = newNode;
			}
		}
		else{
			switch(t->priority){
				case 1:{
					__CRITICAL__ = 1;
					node* newNode = (node*)malloc(sizeof(node));
					__CRITICAL__ = 0;
					newNode->next = NULL;
					newNode->thisThread = t;
					if(queue1->head == NULL)
						queue1->head = newNode;
					else
						queue1->tail->next = newNode;
					queue1->tail = newNode;
					break;
				}
				case 2:{
					__CRITICAL__ = 1;
					node* newNode = (node*)malloc(sizeof(node));
					__CRITICAL__ = 0;
					newNode->next = NULL;
					newNode->thisThread = t;
					if(queue2->head == NULL)
						queue2->head = newNode;
					else
						queue2->tail->next = newNode;
					queue2->tail = newNode;
					break;
				}
				case 3:{
					__CRITICAL__ = 1;
					node* newNode = (node*)malloc(sizeof(node));
					__CRITICAL__ = 0;
					newNode->next = NULL;
					newNode->thisThread = t;
					if(queue3->head == NULL)
						queue3->head = newNode;
					else
						queue3->tail->next = newNode;
					queue3->tail = newNode;
					break;
				}
			}
		}
	}
}

my_pthread* dequeue(){
	//dequeue a new thread to be run
	if(queue3->head!=NULL){
		node *temp = queue3->head;
		if(queue3->tail==queue3->head)
			queue3->tail=queue3->tail->next;
		queue3->head = queue3->head->next;
		my_pthread *dqThread = temp->thisThread;
		__CRITICAL__ = 1;
		free(temp);
		__CRITICAL__ = 0;
		return dqThread;
	}
	else if(queue2->head!=NULL){
		node *temp = queue2->head;
		if(queue2->tail==queue2->head)
			queue2->tail=queue2->tail->next;
		queue2->head = queue2->head->next;
		my_pthread *dqThread = temp->thisThread;
		__CRITICAL__ = 1;
		free(temp);
		__CRITICAL__ = 0;
		return dqThread;
	}
	else if(queue1->head!=NULL){
		node *temp = queue1->head;
		if(queue1->tail==queue1->head)
			queue1->tail=queue1->tail->next;
		queue1->head = queue1->head->next;
		my_pthread *dqThread = temp->thisThread;
		__CRITICAL__ = 1;
		free(temp);
		__CRITICAL__ = 0;
		return dqThread;
	}
	else{
		return NULL;
	}
}

void scheduler(){
	__CRITICAL__ = 1;
	int p = 3;
	if (current_thread != NULL)
	   p = current_thread->priority;

	//enqueue current_thread
	enqueue(current_thread);
	if(current_thread != NULL && current_thread->status == THREAD_DYING)
		current_thread = NULL;

	//running a time slice without finishing lowers your priority
	if(current_thread != NULL && current_thread->priority>1)
		current_thread->priority--;

	node* ptr;
	node* prev;
	prev=NULL;
	for(ptr=queue3->head; ptr!=NULL; prev=ptr, ptr=ptr->next){
		ptr->thisThread->runningTime+=100000*(4-p);
	}
	prev=NULL;
	for(ptr=queue2->head; ptr!=NULL; prev=ptr, ptr=ptr->next){
		ptr->thisThread->runningTime+=100000*(4-p);
		if(ptr->thisThread->runningTime>600000 && ptr->thisThread->priority<3){
			ptr->thisThread->priority++;
			ptr->thisThread->runningTime = 0;
			//change queues
			if(ptr==queue2->head){
				if(queue2->head == queue2->tail){
					queue2->tail = prev;
					if(prev!=NULL)
						queue2->tail->next = NULL;
				}
				queue2->head = queue2->head->next;
				ptr->next = NULL;
				my_pthread* temp = ptr->thisThread;
				free(ptr);
				enqueue(temp);
			}
			else if(ptr==queue2->tail){
				queue2->tail = prev;
				queue2->tail->next = NULL;
				my_pthread* temp = ptr->thisThread;
				free(ptr);
				enqueue(temp);
			}
			else{
				prev->next = ptr->next;
				ptr->next = NULL;
				my_pthread* temp = ptr->thisThread;
				free(ptr);
				enqueue(temp);
			}
		}
	}
	prev=NULL;
	for(ptr=queue1->head; ptr!=NULL; prev=ptr, ptr=ptr->next){
		ptr->thisThread->runningTime+=100000*(4-p);
		if(ptr->thisThread->runningTime>600000 && ptr->thisThread->priority<3){
			ptr->thisThread->priority++;
			ptr->thisThread->runningTime = 0;
			//change queues
			if(ptr==queue1->head){
				if(queue1->head == queue1->tail){
					queue1->tail = prev;
					if(prev!=NULL)
						queue1->tail->next = NULL;
				}
				queue1->head = queue1->head->next;
				ptr->next = NULL;
				my_pthread* temp = ptr->thisThread;
				free(ptr);
				enqueue(temp);
			}
			else if(ptr==queue1->tail){
				queue1->tail = prev;
				queue1->tail->next = NULL;
				my_pthread* temp = ptr->thisThread;
				free(ptr);
				enqueue(temp);
			}
			else{
				prev->next = ptr->next;
				ptr->next = NULL;
				my_pthread* temp = ptr->thisThread;
				free(ptr);
				enqueue(temp);
			}
		}
	}

	//dequeue a new thread to be run
	current_thread = dequeue();

	//unblock the signal
	sigset_t* set = malloc(sizeof(sigset_t));
	sigemptyset (set);
	sigaddset(set, SIGVTALRM);
	sigprocmask(SIG_SETMASK, set, NULL);
	free(set);

	//change time slice based on priority; longer for lower priority threads
	if(current_thread!=NULL){
		struct itimerval period;
		/* set up interrupts interval and period */
		period.it_value.tv_sec = 0;
		period.it_value.tv_usec = 100000*(4-current_thread->priority);
		period.it_interval.tv_sec = 0;
		period.it_interval.tv_usec = 100000*(4-current_thread->priority);
		setitimer(ITIMER_VIRTUAL, &period, NULL);
	}

	if(firstTime){	//protect and unprotect pages
		updateMemoryProtections();
	}
	
	__CRITICAL__ = 0; /* leaving scheduler */
	if(current_thread!=NULL)
		setcontext(current_thread->context);
}

static void handler(int sig, siginfo_t *si, void *unused){
	printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
	exit(EXIT_FAILURE);
}

void interrupt_handler(int sig){
	/* check if the thread is in a critical section */
	if (__CRITICAL__) { return; }
		scheduler();
}

void markDead(){
	current_thread->status = THREAD_DYING;
	scheduler();
}

void thread_init(){
	__CRITICAL__ = 1;
	
	queue1 = malloc(sizeof(queue));
	queue1->head=NULL;
	queue1->tail=NULL;
	queue2 = malloc(sizeof(queue));
	queue2->head=NULL;
	queue2->tail=NULL;
	queue3 = malloc(sizeof(queue));
	queue3->head=NULL;
	queue3->tail=NULL;
	deadQueue = NULL;
	
	struct itimerval period;
	struct sigaction sa;
	/* set up interrupts interval and period */
	period.it_value.tv_sec = 0;
	period.it_value.tv_usec = 100000;
	period.it_interval.tv_sec = 0;
	period.it_interval.tv_usec = 100000;
	/* setup alternative stack */
	sa.sa_flags = SA_ONSTACK;
	/* interrupt handler */
	sa.sa_handler = interrupt_handler;
	if (sigaction(SIGVTALRM, &sa, NULL) < 0){
		/* error checking code */
		return;
	}
	/* Setup signal delivery on separate stack */
	stack_t* ss = malloc(sizeof(stack_t));
	ss->ss_size=SIGSTKSZ;
	ss->ss_flags = 0;
	ss->ss_sp = malloc(sizeof(SIGSTKSZ));
	sigaltstack(ss, NULL);
	setitimer(ITIMER_VIRTUAL, &period, NULL);
	__CRITICAL__ = 0;
}


/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) 
{
	__CRITICAL__ = 1;
	if(thread_inited==0){
		thread_init();
		thread_inited++;
	}

	my_pthread* newThread = malloc(sizeof(my_pthread));
	ucontext_t* newContext = malloc(sizeof(ucontext_t));
	void* newStack = malloc(64*1024);	//not sure how big this should be

	if (getcontext(newContext) == -1){
		perror("Error getting context: Could not get the new context\n");
		exit(1);
	}
	
	if(newStack==((void*)-1)){
		//malloc failed
	exit(EXIT_FAILURE);
		}
	newContext->uc_stack.ss_sp = newStack;
	newContext->uc_stack.ss_size = 64*1024;
	
	ucontext_t* dyingContext = malloc(sizeof(ucontext_t));
	void* dyingStack = malloc(64*1024);
	newContext->uc_stack.ss_size = 64*1024;

	if(dyingStack==((void*)-1)){
		//malloc failed
	exit(EXIT_FAILURE);
		}
	dyingContext->uc_stack.ss_sp = dyingStack;
	  dyingContext->uc_stack.ss_size = 64*1024;
	dyingContext->uc_link = NULL;
	getcontext(dyingContext);
	makecontext(dyingContext, markDead, 0);
	
	newContext->uc_link = dyingContext;
	
	newThread->tid = *thread;
	newThread->status = THREAD_READY;
	newThread->priority = 3;
	newThread->ret = NULL;
	newThread->runningTime = 0;
	getcontext(newContext);
	makecontext(newContext, *function, 1, arg);
	newThread->context = newContext;

	//Put the new thread on the highest priorty queue
	node* newNode = (node*)malloc(sizeof(node));
	newNode->next = NULL;
	newNode->thisThread = newThread;
	if(queue3->head == NULL)
		queue3->head = newNode;
	else
		queue3->tail->next = newNode;
	queue3->tail = newNode;

	__CRITICAL__ = 0;
	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	scheduler();
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) 
{
	__CRITICAL__ = 1;
	current_thread->ret = value_ptr;
	current_thread->status = THREAD_DYING;
	__CRITICAL__ = 0;
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) 
{
	__CRITICAL__ = 1;
	//put this context in the queue to return later
	my_pthread* this = malloc(sizeof(my_pthread));
	this->tid = -1;
	this->status = THREAD_READY;
	this->ret = NULL;
	this->runningTime = 0;
	this->priority = 3;
	this->context = malloc(sizeof(ucontext_t));
	getcontext(this->context);
	
	// if (getcontext(this->context) == -1){
	// 	perror("Error getting context: Could not get the new context\n");
	// 	exit(1);
	// }
	
	enqueue(this);
	__CRITICAL__ = 0;

	node_t* t = NULL;
	int found = 0;
	while(!found){
		node_t* ptr = deadQueue;
		while(ptr!=NULL){
			if(ptr->tid == thread){
				t = ptr;
				found = 1;
				break;
			}
			ptr = ptr->next;
		}
	}
	if(value_ptr != NULL)
		(*value_ptr) = t->ret;
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	__CRITICAL__ = 1;
	mutex->status = MUTEX_UNLOCKED;
	__CRITICAL__ = 0;
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	while(__sync_lock_test_and_set(&(mutex->status), MUTEX_LOCKED) == MUTEX_LOCKED);
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	__CRITICAL__ = 1;
	__sync_lock_test_and_set(&(mutex->status), MUTEX_UNLOCKED);
	__CRITICAL__ = 0;
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	__CRITICAL__ = 1;
	mutex->status = MUTEX_DESTROYED;
	__CRITICAL__ = 0;
	return 0;
};




//combines 4 chars into an int using bitwise operations
static unsigned int fourCharToInt(char a, char b, char c, char d){
	return
	((unsigned char)a<<24)|
	((unsigned char)b<<16)|
	((unsigned char)c<<8)|
	(unsigned char)d;
}

//returns the TID of the given page
static my_pthread_t getPageTid(unsigned int pageNum){
	return fourCharToInt(
		memory[pageNum*PAGE_SIZE+1],
		memory[pageNum*PAGE_SIZE+2],
		memory[pageNum*PAGE_SIZE+3],
		memory[pageNum*PAGE_SIZE+4]
	);
}

//returns the TID of the given page within the swap file
static my_pthread_t getPageTidSwap(unsigned int pageNum){
	return fourCharToInt(
		swapMemory[(pageNum*(PAGE_SIZE)+4)+1],
		swapMemory[(pageNum*(PAGE_SIZE)+4)+2],
		swapMemory[(pageNum*(PAGE_SIZE)+4)+3],
		swapMemory[(pageNum*(PAGE_SIZE)+4)+4]
	);
}

//sets the tid metadata of the given page to the requested value
static void setPageTid(unsigned int pageNum, my_pthread_t tid){
	memory[pageNum*PAGE_SIZE+1] = tid>>24;
	memory[pageNum*PAGE_SIZE+2] = (tid<<8)>>24;
	memory[pageNum*PAGE_SIZE+3] = (tid<<16)>>24;
	memory[pageNum*PAGE_SIZE+4] = (tid<<24)>>24;
}

//sets the tid metadata of the given page in the swap file to the requested value
static void setPageTidSwap(unsigned int pageNum, my_pthread_t tid){
	swapMemory[(pageNum*(PAGE_SIZE)+4)+1] = tid>>24;
	swapMemory[(pageNum*(PAGE_SIZE)+4)+2] = (tid<<8)>>24;
	swapMemory[(pageNum*(PAGE_SIZE)+4)+3] = (tid<<16)>>24;
	swapMemory[(pageNum*(PAGE_SIZE)+4)+4] = (tid<<24)>>24;
}

//returns the size of the metadata pointed to by i
static unsigned int getBlockSize(int i){
	return fourCharToInt(memory[i+1], memory[i+2], memory[i+3], memory[i+4]);
}

//returns the size of the metadata pointed to by i in the swap file
static unsigned int getBlockSizeSwap(int i){
	return fourCharToInt(swapMemory[i+1], swapMemory[i+2], swapMemory[i+3], swapMemory[i+4]);
}

//sets the metadata pointed to by i's size equal to capacity
static void setBlockSize(int i, unsigned int capacity){
	my_pthread_t cap = capacity;
	memory[i+1] = cap>>24;
	memory[i+2] = (cap<<8)>>24;
	memory[i+3] = (cap<<16)>>24;
	memory[i+4] = (cap<<24)>>24;
}

//sets the metadata pointed to by i's size equal to capacity in the swap file
static void setBlockSizeSwap(int i, unsigned int capacity){
	my_pthread_t cap = capacity;
	swapMemory[i+1] = cap>>24;
	swapMemory[i+2] = (cap<<8)>>24;
	swapMemory[i+3] = (cap<<16)>>24;
	swapMemory[i+4] = (cap<<24)>>24;
}

//checks if the metadata pointed to by i is marked as allocated
static char isAllocated(int i){
	return memory[i] == 1;
}

//checks if the metadata pointed to by i is marked as allocated in the swap file
static char isAllocatedSwap(int i){
	return swapMemory[i] == 1;
}

//checks if the given page has an unallocated block of size capacity or greater
static char hasSpace(int pageNum, unsigned int capacity){
	int i = pageNum*PAGE_SIZE+5;
	if(pageNum>=MEMORY_SIZE/PAGE_SIZE-4)
		i = pageNum*PAGE_SIZE;
	for(i=i; i<(pageNum+1)*PAGE_SIZE; i+=getBlockSize(i)+5){
		if(!isAllocated(i) && getBlockSize(i)>=capacity){
			return 1;
		}
	}
	return 0;
}

//checks if the given page in the swap file has an unallocated block of size capacity or greater
static char hasSpaceSwap(int pageNum, unsigned int capacity){
	int i;
	for(i=pageNum*PAGE_SIZE+9; i<(pageNum+1)*PAGE_SIZE; i+=getBlockSizeSwap(i)+5){
		if(!isAllocatedSwap(i) && getBlockSizeSwap(i)>=capacity){
			return 1;
		}
	}
	return 0;
}

//our implementation of malloc
void* myallocate(int capacity, char* file, int line, char threadreq){
	
	int oldCrit = __sync_val_compare_and_swap(&__CRITICAL__, 0, 1);

	int i;
	
	if(!firstTime)
		setupMemory();

	mprotect(memory, MEMORY_SIZE, PROT_READ | PROT_WRITE);
	
	if(capacity<0){
		fprintf(stderr, "Error on malloc in file: %s,  on line %d. Cannot allocate a negative amount of memory\n", file, line);
		updateMemoryProtections();
		__CRITICAL__ = oldCrit;
		return NULL;
	}
	
	my_pthread_t curr = -1;
	if(current_thread != NULL)
		curr = current_thread->tid;

	int temp;
	for(i=0; i<MEMORY_SIZE/PAGE_SIZE-4; i++){	//try to find an open unshared page
		temp = getPageTid(i);
		if(!isAllocated(i*PAGE_SIZE) || (temp == curr && hasSpace(i, capacity))){
			break;
		}
	}
	if(i==MEMORY_SIZE/PAGE_SIZE-4){   //out of non shared pages

	  //finding first spot that doesnt use that thread ID to evict
	  for (i = 0; i < MEMORY_SIZE/ PAGE_SIZE - 4; i++)
	    if(getPageTid(i) != curr)
	      break;
		
	  //finding open spot in swapfile
	  int j;
	  for(j = 0; j < 2*MEMORY_SIZE / PAGE_SIZE - 1; j++){
	    temp = getPageTidSwap(j);
	    if(!isAllocatedSwap(j*PAGE_SIZE) || (temp == curr && hasSpaceSwap(j,capacity)))
	      break;

	    if(j == MEMORY_SIZE / PAGE_SIZE - 1)     //out of swapfile pages
	      {
		updateMemoryProtections();
		__CRITICAL__ = oldCrit;
		return NULL;
	      }
	    int k;
	    for(k = 0; k <PAGE_SIZE; k++)
	      swapMemory[j+k] = memory[i+k];
	  }

	}
	if(!isAllocated(i*PAGE_SIZE)){	//unallocated page, give it our tid & mark as allocated
		setPageTid(i, curr);
		memory[i*PAGE_SIZE] = 1;
	}
	
	//allocate within page i
	temp = i;
	for(i=i*PAGE_SIZE+5; i<(temp+1)*PAGE_SIZE; i+=getBlockSize(i)+5){
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

	updateMemoryProtections();
	__CRITICAL__ = oldCrit;

	return &memory[i+5];
}

//our implementation of free
void mydeallocate(void* toBeFreed, char* file, int line, char threadreq){
	int oldCrit = __sync_val_compare_and_swap(&__CRITICAL__, 0, 1);

	if(mprotect(memory, MEMORY_SIZE, PROT_READ | PROT_WRITE)){
		perror("Couldn't use mprotect");	
	}

	//check if pointer to be freed is within the memory block
	int difference = toBeFreed - ((void*)memory);
	if(difference < 0 || difference > MEMORY_SIZE){
		fprintf(stderr, "Error on free in file: %s,  on line %d. Not within memory block\n", file, line);
		updateMemoryProtections();
		__CRITICAL__ = oldCrit;
		return;
	}

	//check if the thread calling free is allowed to access this page
	int pageItsIn = difference / PAGE_SIZE;
	int i;
	//any thread can free shared memory
	char shared = 0;
	if((MEMORY_SIZE / PAGE_SIZE) - pageItsIn > 4){
		if(current_thread->tid != getPageTid(pageItsIn)){
			fprintf(stderr, "Error on free in file: %s, on line %d. Page blocked from thread.\n", file, line);
			updateMemoryProtections();
			__CRITICAL__ = oldCrit;
			return;
		}
	}
	else
		shared = 1;

	i = pageItsIn * PAGE_SIZE + 5;
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
				updateMemoryProtections();
				__CRITICAL__ = oldCrit;
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
		memory[pageItsIn*PAGE_SIZE] = 1;
	}

	if(!found)
		fprintf(stderr, "Error on free in file: %s, on line %d. Pointer not reference to beginning of allocated block.\n", file, line);
	updateMemoryProtections();
	__CRITICAL__ = oldCrit;
}

//allocates a chunk of shared memory of the given size
void* shalloc(size_t size){
	int oldCrit = __sync_val_compare_and_swap(&__CRITICAL__, 0, 1);

	if(!firstTime)
		setupMemory();

	if(mprotect(memory, MEMORY_SIZE, PROT_READ | PROT_WRITE)){
		perror("Couldn't use mprotect");	
	}

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
					int currSize = getBlockSize(i+5+size);
					int nextSize = getBlockSize(nextI);
					setBlockSize(i+5+size, currSize+5+nextSize);
				}
			}
			//otherwise, leave the user oldSize size to avoid complications
			updateMemoryProtections();
			__CRITICAL__ = oldCrit;
			return &memory[i+5];
		}
	}

	updateMemoryProtections();
	__CRITICAL__ = oldCrit;
	return NULL;
}

#define malloc(x) myallocate(x,__FILE__,__LINE__,THREADREQ)
#define free(x) mydeallocate (x,__FILE__,__LINE__,THREADREQ)
