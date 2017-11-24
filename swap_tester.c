#include "my_pthread.c"
#include <stdio.h>

static int x = 0, y = 0;
static my_pthread_mutex_t l;
static char xp1Done = 0;
static char yDone = 0;

//fill up memory
void* test(void* arg){
	int numToMake = (int)(.75*(((MEMORY_SIZE/PAGE_SIZE)-4)));
	if(arg==&x){
		my_pthread_mutex_lock(&l);
		char *xAllocs[numToMake];
		while(*((int*)arg) < numToMake){
			xAllocs[*((int*)arg)] = malloc(PAGE_SIZE-10);
			int j;
			//test protection after allocation
			for(j=0; j<PAGE_SIZE-10; j++){
				xAllocs[*((int*)arg)][j] = '\0';
			}
			printf("X: %d\n", *((int*)arg)+1);
			(*((int*)arg))++;
			if(*((int*)arg)+1 == numToMake){
				xp1Done = 1;
				my_pthread_mutex_unlock(&l);
				while(!yDone){}
				my_pthread_mutex_lock(&l);
			}
		}
		int i;
		//test protection after swap
		for(i=0; i<numToMake; i++){
			int j;
			for(j=0; j<PAGE_SIZE-10; j++){
				xAllocs[i][j] = 'X';
			}
		}
		my_pthread_mutex_unlock(&l);
	}

	else{
		while(!xp1Done){}
		my_pthread_mutex_lock(&l);
		char *yAllocs[numToMake];
		while(*((int*)arg) < numToMake){
			yAllocs[*((int*)arg)] = malloc(PAGE_SIZE-10);
			int j;
			//test protection after allocation
			for(j=0; j<PAGE_SIZE-10; j++){
				yAllocs[*((int*)arg)][j] = '\0';
			}
			printf("Y: %d\n", *((int*)arg)+1);
			(*((int*)arg))++;
		}
		int i;
		//test protection after all allocations
		for(i=0; i<numToMake; i++){
			int j;
			for(j=0; j<PAGE_SIZE-10; j++){
				yAllocs[i][j] = 'Y';
			}
		}
		yDone = 1;
		my_pthread_mutex_unlock(&l);
	}
	return NULL;
}

int main(){
	my_pthread_t xt = 1;
	my_pthread_t yt = 2;

	int numToMake = (int)(.75*(((MEMORY_SIZE/PAGE_SIZE)-4)));

	my_pthread_mutex_init(&l, NULL);

	my_pthread_create(&xt, NULL, &test, &x);
	my_pthread_create(&yt, NULL, &test, &y);

	my_pthread_join(xt, NULL);
	my_pthread_join(yt, NULL);

	my_pthread_mutex_destroy(&l);

	printf("Done\nPages created: X=%d; Y=%d; Total=%d; Expected Total=%d\n",
		x, y, x+y, 2*numToMake);
	printf("MEMORY_SIZE: %d\nPAGE_SIZE: %d\nM/P=%d\n", MEMORY_SIZE, (int)PAGE_SIZE, (int)(MEMORY_SIZE/PAGE_SIZE));

	return 0;
}
