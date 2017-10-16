// File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	09/23/2017

// name: Nicholas Prezioso, Benjamin Cahnbley, and Marcella Alvarez
// username of iLab: njp107, bc499, and ma1143
// iLab Server: prototype
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

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

enum thread_status{
	THREAD_RUNNING,     /* Running thread. */
	THREAD_READY,       /* Not running but ready to run. */
	THREAD_BLOCKED,     /* Waiting for an event to trigger. */
	THREAD_DYING        /* About to be destroyed. */
};

typedef uint my_pthread_t;

//need macros to prevent use of regular pthread and mutex stuff

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

#endif
