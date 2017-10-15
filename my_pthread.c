// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name: Nicholas Prezioso, Benjamin Cahnbley, and Marcella Alvarez
// username of iLab: njp107, bc499,  and ma1143
// iLab Server: prototype

#include "my_pthread_t.h"
#define MAX_THREADS 101


static int thread_inited = 0;
/*Q Max for Threads*/
static queue* queue1;
static queue* queue2;
static queue* queue3;
static node_t* deadQueue;
/* The pid of the parent process */
static pid_t parentPid;
/* The number of active threads */
static int numThreads = 0;
/*in scheduler or allocating memory*/
static int __CRITICAL__ = 0;

static ucontext_t mainContext;
static my_pthread* current_thread = NULL;


int enqueue(my_pthread* t){
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

//void scheduler(struct sigcontext *scp) {
void scheduler() {
	__CRITICAL__ = 1;
	int p = 3;
	if (current_thread != NULL)
	  int p = current_thread->priority;

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
	
	__CRITICAL__ = 0; /* leaving scheduler */
	if(current_thread!=NULL)
		setcontext(current_thread->context);
}

//void interrupt_handler(int sig, int code, struct sigcontext *scp) {
void interrupt_handler(int sig) {
	/* check if the thread is in a critical section */
	if (__CRITICAL__) { return; }
		scheduler();
}

void markDead(){
	current_thread->status = THREAD_DYING;
}

void thread_init(){
	parentPid = getpid();
	
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
	__CRITICAL__ = 0;
	
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
	__CRITICAL__ = 1;
	stack_t* ss = malloc(sizeof(stack_t));
	ss->ss_size=SIGSTKSZ;
	ss->ss_flags = 0;
	ss->ss_sp = malloc(sizeof(SIGSTKSZ));
	__CRITICAL__ = 0;
	sigaltstack(ss, NULL);
	setitimer(ITIMER_VIRTUAL, &period, NULL);
}


/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) 
{
	if(thread_inited==0){
		thread_init();
		thread_inited++;
	}

	__CRITICAL__ = 1;
	my_pthread* newThread = malloc(sizeof(my_pthread));
	ucontext_t* newContext = malloc(sizeof(ucontext_t));
	void* newStack = malloc(20000);	//not sure how big this should be

	if(newStack==((void*)-1)){
		//malloc failed
	exit(EXIT_FAILURE);
		}
	__CRITICAL__ = 0;
	newContext->uc_stack.ss_sp = newStack;
	newContext->uc_stack.ss_size = 20000;
	
	__CRITICAL__ = 1;
	ucontext_t* dyingContext = malloc(sizeof(ucontext_t));
	void* dyingStack = malloc(20000);
	newContext->uc_stack.ss_size = 20000;

	if(dyingStack==((void*)-1)){
		//malloc failed
	exit(EXIT_FAILURE);
		}
	__CRITICAL__ = 0;
	dyingContext->uc_stack.ss_sp = dyingStack;
	dyingContext->uc_stack.ss_size = 20000;
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
	__CRITICAL__ = 1;
	node* newNode = (node*)malloc(sizeof(node));
	__CRITICAL__ = 0;
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
	scheduler();
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) 
{
	current_thread->ret = value_ptr;
	current_thread->status = THREAD_DYING;
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) 
{
	node_t* t = NULL;
	int found = 0;
	while(!found){
		node_t* ptr = deadQueue;
		while(ptr!=NULL){
			if(ptr->tid == t->tid){
				t = ptr;
				found = 1;
				break;
			}
			ptr = ptr->next;
		}
	}
	(*value_ptr) = t->ret;
	printf("pooop\n");
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	__CRITICAL__ = 1;
	mutex = malloc(sizeof(mutex));
	mutex->status = malloc(sizeof(enum mutex_status));
	mutex->status  = MUTEX_UNLOCKED;
	__CRITICAL__ = 0;
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	while(/*mutex_status*/__sync_lock_test_and_set((mutex->status), MUTEX_LOCKED) == MUTEX_LOCKED);
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	__CRITICAL__ = 1;
	/*mutex_status*/__sync_lock_test_and_set((mutex->status), MUTEX_UNLOCKED);
	__CRITICAL__ = 0;
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	__CRITICAL__ = 1;
	free(mutex->status);
	free(mutex);
	__CRITICAL__ = 0;
	return 0;
};

