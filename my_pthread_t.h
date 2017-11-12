// File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	09/23/2017

// name: Nicholas Prezioso, Benjamin Cahnbley, and Marcella Alvarez
// username of iLab: njp107, bc499, and ma1143
// iLab Server: prototype
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* To use real pthread Library in Benchmark, you have to comment the USE_MY_PTHREAD macro */
#define USE_MY_PTHREAD 1

#define MEMORY_SIZE 1048576
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)

#define THREADREQ 0
#define LIBRARYREQ 1



/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ucontext.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>
#include <sys/mman.h>
#include <fcntl.h>

enum thread_status{
	THREAD_RUNNING,     /* Running thread. */
	THREAD_READY,       /* Not running but ready to run. */
	THREAD_BLOCKED,     /* Waiting for an event to trigger. */
	THREAD_DYING        /* About to be destroyed. */
};

static char* memory;
static char* swapMemory;
static int fd;
static char firstTime;

typedef uint my_pthread_t;

//need macros to prevent use of regular pthread and mutex stuff
/*#define pthread_create( w, x, y, z ) my_pthread_create( w, x, y, z )
#define pthread_yield( ) my_pthread_yield( )
#define pthread_exit( x ) my_pthread_exit( x )
#define pthread_join( x, y ) my_pthread_join( x, y )
#define pthread_mutex_init( x, y ) my_pthread_mutex_init( x, y )
#define pthread_mutex_lock( x ) my_pthread_mutex_lock( x )
#define pthread_mutex_unlock( x ) my_pthread_mutex_unlock( x )
#define pthread_mutex_destroy( x ) my_pthread_mutex_destroy( x )*/

typedef struct my_pthread_{
	my_pthread_t tid;
	enum thread_status status;
	ucontext_t* context;
	int priority;
	void* ret;
	long double runningTime;
} my_pthread;

typedef struct threadControlBlock {

} tcb;

enum mutex_status{
	MUTEX_DESTROYED,
	MUTEX_UNLOCKED,
	MUTEX_LOCKED
};


 typedef struct node_{
	my_pthread* thisThread;
 	struct node_* next;
 } node;

 typedef struct node_t_{
 	my_pthread_t tid;
 	void* ret;
 	struct node_t_* next;
 } node_t;
 
 
 typedef struct queue_{
    node* head;
    node* tail;
 } queue;

/* mutex struct definition */
typedef struct my_pthread_mutex_t {
	enum mutex_status status;
} my_pthread_mutex_t;

/* define your data structures here: */

// Feel free to add your own auxiliary data structures


/* Function Declarations: */

void updateMemoryProtections();

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

static int fourCharToInt(char a, char b, char c, char d);

static my_pthread_t getPageTid(int pageNum);

static void setPageTid(int pageNum, my_pthread_t tid);

static int getBlockSize(int i);

static void setBlockSize(int i, int capacity);

static char hasSpace(int pageName, int capacity);

void* myallocate(int capacity, char* file, int line, char threadreq);

void mydeallocate(void* toBeFreed, char* file, int line, char threadreq);

void* shalloc(size_t size);

#ifdef USE_MY_PTHREAD
#define pthread_t my_pthread_t
#define pthread_mutex_t my_pthread_mutex_t
#define pthread_create my_pthread_create
#define pthread_exit my_pthread_exit
#define pthread_join my_pthread_join
#define pthread_mutex_init my_pthread_mutex_init
#define pthread_mutex_lock my_pthread_mutex_lock
#define pthread_mutex_unlock my_pthread_mutex_unlock
#define pthread_mutex_destroy my_pthread_mutex_destroy

#define malloc(x) myallocate(x,__FILE__,__LINE__,THREADREQ)
#define free(x) mydeallocate (x,__FILE__,__LINE__,THREADREQ)
#endif

#endif
