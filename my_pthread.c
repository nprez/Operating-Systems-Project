// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name: Nicholas Prezioso, Benjamin Cahnbley, and Marcella Alvarez
// username of iLab: njp107, bc499, and ma1143
// iLab Server: prototype

#include "my_pthread_t.h"
#define MAX_THREADS 101


static int thread_inited = 0;
/*Q Max for Threads*/
static queue* queue1;
static queue* queue2;
static queue* queue3;
/* The pid of the parent process */
static pid_t parentPid;
/* The number of active threads */
static int numThreads = 0;

void thread_init(){
	parentPid = getpid();

	queue1 = malloc(sizeof(queue));
	queue1->head=NULL;
	queue1->tail=NULL;
	queue2 = malloc(sizeof(queue));
	queue2->head=NULL;
	queue2->tail=NULL;
	queue3 = malloc(sizeof(queue));
	queue3->head=NULL;
	queue3->tail=NULL;
}


/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) 
{
	my_pthread* newThread = malloc(sizeof(my_pthread));
	ucontext_t* newContext = malloc(sizeof(ucontext_t));
	char newStack[20000];	//not sure how big this should be
	newContext->uc_stack.ss_sp = newStack;
	newContext->uc_stack.ss_size = sizeof(newStack);
	newContext->uc_link = NULL;
	newThread->tid = *thread;
	newThread->status = THREAD_READY;
	newThread->priority = 3;
	makecontext(newContext, *function, 1, arg);
	newThread->context = newContext;

	//Put the new thread on the highest priorty queue
	node* newNode = (node*)malloc(sizeof(node)); 
	newNode->next = NULL;
	newNode->thisThread = newThread;
	if(queue3->head == NULL){
	  queue3->head = newNode;
	  queue3->tail = newNode;
	}
	else{
		queue3->tail->next = newNode;
		queue3->tail = newNode;
	}
	
	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	mutex = malloc(sizeof(mutex));
	mutex->status = malloc(sizeof(enum mutex_status));
	mutex->status  = MUTEX_UNLOCKED;
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	while(mutex_status__sync_lock_test_and_set(mutex->status, MUTEX_LOCKED) == MUTEX_LOCKED);
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	mutex_status__sync_lock_test_and_set(mutex->status, MUTEX_UNLOCKED);
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	free(mutex->status);
	free(mutex);
	return 0;
};

