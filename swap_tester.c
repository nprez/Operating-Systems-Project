#include "my_pthread.c"
#include <stdio.h>

static int x = 0, y = 0;
static my_pthread_mutex_t l;
static char xp1Done = 0;
static char yDone = 0;

//fill up memory
void* test(void* arg){
	if(arg==&x){
		my_pthread_mutex_lock(&l);
		while(*((int*)arg) < (int)(.75*(((MEMORY_SIZE/PAGE_SIZE)-4)))){
			malloc(PAGE_SIZE-10);
			printf("X: %d\n", *((int*)arg)+1);
			(*((int*)arg))++;
			if(*((int*)arg)+1 == (int)(.75*((MEMORY_SIZE/PAGE_SIZE)-4))){
				xp1Done = 1;
				my_pthread_mutex_unlock(&l);
				while(!yDone){}
				my_pthread_mutex_lock(&l);
			}
		}
		my_pthread_mutex_unlock(&l);
	}

	else{
		while(!xp1Done){}
		my_pthread_mutex_lock(&l);
		while(*((int*)arg) < (int)(.75*(((MEMORY_SIZE/PAGE_SIZE)-4)))){
			malloc(PAGE_SIZE-10);
			printf("Y: %d\n", *((int*)arg)+1);
			(*((int*)arg))++;
		}
		yDone = 1;
		my_pthread_mutex_unlock(&l);
	}
	return NULL;
}

int main(){
	my_pthread_t xt = 1;
	my_pthread_t yt = 2;

	my_pthread_mutex_init(&l, NULL);

	my_pthread_create(&xt, NULL, &test, &x);
	my_pthread_create(&yt, NULL, &test, &y);

	my_pthread_join(xt, NULL);
	my_pthread_join(yt, NULL);

	my_pthread_mutex_destroy(&l);

	printf("Done\nPages created: X=%d; Y=%d; Total=%d; Expected Total=%d\n",
		x, y, x+y, (int)(((MEMORY_SIZE/PAGE_SIZE)-4)*1.5));
	printf("MEMORY_SIZE: %d\nPAGE_SIZE: %d\nM/P=%d\n", MEMORY_SIZE, (int)PAGE_SIZE, (int)(MEMORY_SIZE/PAGE_SIZE));

	return 0;
}
