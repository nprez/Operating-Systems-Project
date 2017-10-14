// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name: Nicholas Prezioso and Marcella Alvarez
// username of iLab: njp107 and ma1143
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
/*in scheduler or allocating memory*/
static int __CRITICAL__ = 0;

static my_pthread* current_thread = NULL;

void scheduler(struct sigcontext *scp) {
	__CRITICAL__ = 1;

	//enqueue current_thread
	if(current_thread!=NULL){
		switch(current_thread->priority){
			case 1:{
				node* newNode = (node*)malloc(sizeof(node));
				newNode->next = NULL;
				newNode->thisThread = current_thread;
				if(queue1->head == NULL)
					queue1->head = newNode;
				else
					queue1->tail->next = newNode;
				queue1->tail = newNode;

				break;
			}
			case 2:{
				node* newNode = (node*)malloc(sizeof(node));
				newNode->next = NULL;
				newNode->thisThread = current_thread;
				if(queue2->head == NULL)
					queue2->head = newNode;
				else
					queue2->tail->next = newNode;
				queue2->tail = newNode;

				break;
			}
			case 3:{
				node* newNode = (node*)malloc(sizeof(node));
				newNode->next = NULL;
				newNode->thisThread = current_thread;
				if(queue3->head == NULL)
					queue3->head = newNode;
				else
					queue3->tail->next = newNode;
				queue3->tail = newNode;

				break;
			}
		}
	}
	
	//dequeue a new thread to be run
	if(queue3->head!=NULL){
		if(queue3->tail==queue3->head)
			queue3->tail = queue3->tail->next;
		current_thread=queue3->head->thisThread;
		queue3->head = queue3->head->next;
	}
	else if(queue2->head!=NULL){
		if(queue2->tail==queue2->head)
			queue2->tail = queue2->tail->next;
		current_thread=queue2->head->thisThread;
		queue2->head = queue2->head->next;
	}
	else if(queue1->head!=NULL){
		if(queue1->tail==queue1->head)
			queue1->tail = queue1->tail->next;
		current_thread=queue1->head->thisThread;
		queue1->head = queue1->head->next;
	}
	else{
		current_thread = NULL;
	}

	//sigsetmask(scp->sc_mask); /* unblocks signal */
	sigprocmask(SIG_SETMASK, scp->sc_mask, NULL);
	__CRITICAL__ = 0; /* leaving scheduler */
	
	if(current_thread!=NULL)
		setcontext(current_thread->context);
}

void interrupt_handler(int sig, int code, struct sigcontext *scp) {
	/* check if the thread is in a critical section */
	if (__CRITICAL__) { return; }
	scheduler(scp);
}

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
	//sigaltstack* ss = malloc(sizeof(sigaltstack));
	ss->ss_size=SIGSTKSZ;
	ss->ss_flags = 0;
	ss->ss_sp = malloc(sizeof(SIGSTKSZ));
	sigaltstack(ss, NULL);
	setitimer(ITIMER_VIRTUAL, &period, NULL);
}


/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) 
{
	if(thread_inited=0){
	thread_init();
	thread_inited++;
	}
	
	my_pthread* newThread = malloc(sizeof(my_pthread));
	ucontext_t* newContext = malloc(sizeof(ucontext_t));
	char newStack[20000];	//not sure how big this should be
	newContext->uc_stack.ss_sp = newStack;
	newContext->uc_stack.ss_size = sizeof(newStack);
	newContext->uc_link = &scheduler;
	newThread->tid = *thread;
	newThread->status = THREAD_READY;
	newThread->priority = 3;
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

