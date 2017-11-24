#include "my_pthread.c"
#include <stdio.h>

my_pthread_mutex_t xlock;
my_pthread_mutex_t ylock;

void* inc_x(void *x_void_ptr){
	/* increment x to 100 */
	my_pthread_mutex_lock(&xlock);
	int *x_ptr = (int*)malloc(sizeof(int));
	int i = 0;
	while(i<100){
		
		(*x_ptr)++;
		
		i++;
	};
	
printf("x ptr is: %d\n", x_ptr);


void *m = malloc(PAGE_SIZE+1);
 memset(m,0,(PAGE_SIZE+1));

printf("large allocation set\n");


int *x2_ptr = (int*)malloc(sizeof(int));

i=0;
	while(i<200){
		
		(*x2_ptr)++;
		
		i++;
	};

	printf("x ptr2 is: %d\n", x2_ptr);

printf("about to free\n");
free(m);

printf("now free\n");
	
	my_pthread_mutex_unlock(&xlock);
	
	return NULL;
}

void* inc_y(void *y_void_ptr){

	/* increment y to 100 */
	int *y_ptr = (int *)y_void_ptr;
	int i = 0;
	while(i<100){
		my_pthread_mutex_lock(&ylock);
		(*y_ptr)++;
		my_pthread_mutex_unlock(&ylock);
		i++;
	};

	return NULL;
}

int main(){
	int THREAD_NUM = 20;
	my_pthread_t threads[THREAD_NUM];

	int x = 0, y = 0;
	my_pthread_mutex_init(&xlock, NULL);
	my_pthread_mutex_init(&ylock, NULL);

	int i;

	for(i=0; i<THREAD_NUM; i+=2){
		my_pthread_create(&threads[i], NULL, &inc_x, &x);
		my_pthread_create(&threads[i+1], NULL, &inc_y, &y);
	}

	for(i=0; i<THREAD_NUM; i++){
		my_pthread_join(threads[i], NULL);
	}

	my_pthread_mutex_destroy(&xlock);
	my_pthread_mutex_destroy(&ylock);

	printf("Final values: x: %d y: %d\n", x, y);	//1000, 1000
	
	return 0;
}
