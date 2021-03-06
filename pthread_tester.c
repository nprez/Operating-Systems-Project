#include "my_pthread.c"
#include <stdio.h>

/* this function is run by the second thread */
void *inc_x(void *x_void_ptr){

	/* increment x to 100 */
	int *x_ptr = (int *)x_void_ptr;
	while(++(*x_ptr) < 100);

	printf("x increment finished\n");

	/* the function must return something - NULL will do */
	return NULL;
}


int main(){

	int x = 0, y = 0;

	/* show the initial values of x and y */
	printf("x: %d, y: %d\n", x, y);

	/* this variable is our reference to the second thread */
	my_pthread_t inc_x_thread;

	/* create a second thread which executes inc_x(&x) */
	int ret = my_pthread_create(&inc_x_thread, NULL, inc_x, &x);
	if(ret != 0) {
		fprintf(stderr, "Error creating thread\nError code: %d\n", ret);
		return 1;
	}
	/* increment y to 100 in the first thread */
	while(++y < 100);

	printf("y increment finished\n");

	/* wait for the second thread to finish */
	printf(" In the end of the thread: x: %d, y: %d\n", x, y);
	ret = my_pthread_join(inc_x_thread, NULL);
	printf(" In the end of the thread: x: %d, y: %d\n", x, y);
	
	if(ret != 0) {
		fprintf(stderr, "Error joining thread\nError Code: %d\n", ret);
		return 2;
	}

	/* show the results - x is now 100 thanks to the second thread */
	printf("x: %d, y: %d\n", x, y);

	return 0;
}


/*

This code SHOULD print something like:

x: 0, y:0
y increment finished
x increment finished
x: 100, y: 100


*/
