CS416 - Assignment 1


*ALL Critical Sections within my_pthread_create cannot be interrupted by anything, including the scheduler (Noted by _CRITICAL_)

Methods:

void enqueue(my_pthread* t);
  Enqueues thread into the correct queue by priority
  
my_pthread* dequeue();
  Find a queue to dequeue, starts with searching the highest priority queue and then continues downwards.
  
void thread_init();
  Initializes my_pthread, called when my_pthread_create is called, and is only done once.

int my_pthread_create( my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);
  Creates a thread and adds it to the highest priority queue

void scheduler();
  In charge of changing threads and time slices

void my_pthread_yield();
  Call to the scheduler requesting that the thread (AKA context) can be swapped out for another
 
void pthread_exit(void *value_ptr);
  Call to end the pthread that called it. Saves value_ptr
 
int my_pthread_join(my_pthread_t thread, void **value_ptr);
  Call making sure that the parent/calling thread will not continue until the one given is done. Saves value_ptr
 
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);
  Initializes a mutex created by the calling thread, status noted as MUTEX_UNLOCKED
 
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);
  Locks the given mutex, noted by MUTEX_LOCKED
 
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);
Unlocks a given mutex, noted by MUTEX_UNLOCKED
 
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);
  Destroys a given mutex, noted by MUTEX_DESTROYED

void interrupt_handler(int sig);
  Checks is a thread is in a critical section, if not, scheduler is called
  
 void markDead();
  Marks thread as dying, status noted as THREAD_DYING
  
  


Structure:

  There are queues for each priority level-- high (queue3), medium (queue2) and low (queue1).  Their context is saved, and each running thread is identified by current_thread. This is the thread struct that changes contexts to run other threads.

Extra Credit: Solving Priority Inversion

As a user space thread program, only one thread is running at a time realistically, and therefore only one mutex is in use. Priority Inversion is solved because the highest priority thread will always take priority (i.e. run for its time, until priority is decreased). If a low priority thread is running, once it completes, the thread is not given to a medium priority, it goes straight to a high priority thread (if there are no high priority it will go to medium priority only after it's checked for high priority). Therefore, priority inheritance is not needed as before a tread is picked, it must search through the top most priority thread and go down in descending order (if there are no highest priority threads in the queue).
