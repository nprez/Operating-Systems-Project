#include "my_pthread.c"
#include <stdio.h>

static int x = 0, y = 0;
static char* s0;
static char* s1;

//fill up memory
void* test(void* arg){
	while(*((int*)arg) < (((MEMORY_SIZE/PAGE_SIZE)-4)/2)){
		char* ret = malloc(PAGE_SIZE-10);
		if(arg == &x)
			ret[0] = 'X';
		else
			ret[0] = 'Y';
		ret[1] = '\0';
		//test if memory is accessible
		int i;
		for(i=2; i<PAGE_SIZE-10; i++)
			ret[i] = '\0';
		printf("%s: %d\n", ret, *((int*)arg)+1);
		(*((int*)arg))++;
	}
	char* sh = shalloc((2*PAGE_SIZE)-5);
	sh[0] = 'S';
	if(arg==&x)
		sh[1] = 'X';
	else
		sh[1] = 'Y';
	//test if shared memory is accessible
	int i;
	for(i=2; i<((2*PAGE_SIZE)-5); i++)
		sh[i] = '\0';
	printf("%s: %d\n", sh, arg==&y);
	printf("Shared memory size %d created\n", (int)((2*PAGE_SIZE)-5));
	if(arg==&x)
		s0 = sh;
	else
		s1 = sh;
	return NULL;
}

int main(){
	my_pthread_t xt = 1;
	my_pthread_t yt = 2;

	my_pthread_create(&xt, NULL, &test, &x);
	my_pthread_create(&yt, NULL, &test, &y);

	my_pthread_join(xt, NULL);
	my_pthread_join(yt, NULL);

	printf("Trying to access S0(X) in main: %s\n", s0);
	printf("Trying to access S1(Y) in main: %s\n", s1);

	printf("Done\nPages created: X=%d; Y=%d; Shared=%d; Total=%d; Expected Total=%d (2 less than total possible)\n",
		x, y, 2, x+y+2, (int)((MEMORY_SIZE/PAGE_SIZE)-4)+2);
	printf("MEMORY_SIZE: %d\nPAGE_SIZE: %d\nM/P=%d\n", MEMORY_SIZE, (int)PAGE_SIZE, (int)(MEMORY_SIZE/PAGE_SIZE));

	return 0;
}
