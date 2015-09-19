#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/times.h>
#include <sys/time.h>
#include <unistd.h>
#include <float.h>
#define N (2)
#define PARALLEL
#define BAL

static double sec(void)
{
	struct timeval t; 
	gettimeofday(&t,NULL);
	return t.tv_sec+t.tv_usec/1000000.0;
	// return clock()/(double)CLOCKS_PER_SEC;
}

static int cmp(const void* ap, const void* bp)
{	
	double d = (*(double*)ap - *(double*)bp);
	if (d > 0) {
		return 1;
	} else if (d < 0){
		return -1;
	} else{
		return 0;
	}
}

typedef struct sarg {
	void*		base;	// Array to sort.
	size_t		n;	// Number of elements in base.
} sarg;

void* work(void* arg){

	printf("work %zu\n",((sarg*)arg)->n);
	qsort(((sarg*)arg)->base, ((sarg*)arg)->n, sizeof(double), cmp);
	return NULL;
}

void par_sort(
	void*		base,	// Array to sort.
	size_t		n,	// Number of elements in base.
	size_t		s,	// Size of each element.
	int		(*cmp)(const void*, const void*)) // Behaves like strcmp
{

#ifdef BAL
	double p = RAND_MAX/2;
#else
	double p = ((double*)base)[0];
#endif
	
	double t;
	double* b1;
	double* b2;
	b1 = malloc(n * sizeof(double));
	b2 = malloc(n * sizeof(double));
	size_t n1 = 0;
	size_t n2 = 0;
	for (int i = 0; i < n; ++i)
	{
		t = ((double*)base)[i];
		if(t>p){
			b1[n1]=t;
			n1++;
		}else{
			b2[n2]=t;
			n2++;
		}
	}
	sarg s1 = {b1,n1};
	sarg s2 = {b2,n2};

	pthread_t pt[N];
	if (pthread_create(&pt[0], NULL, work, &s1) != 0)
		printf("err create\n");
	if (pthread_create(&pt[1], NULL, work, &s2) != 0)
		printf("err create\n");
	for (int i = 0; i < N; ++i)
	{
		if(pthread_join(pt[i],NULL)!=0)
			printf("err join\n");
	}
	memcpy(base,b2,n2*sizeof(double));
	memcpy(&((double*)base)[n2],b1,n1*sizeof(double));
	free(b1);
	free(b2);
}


int main(int ac, char** av)
{
	int		n = 2000000;
	int		i;
	double*		a;
	double		start, end;

	if (ac > 1)
		sscanf(av[1], "%d", &n);

	srand(getpid());

	a = malloc(n * sizeof a[0]);
	for (i = 0; i < n; i++)
		a[i] = rand();

	start = sec();

#ifdef PARALLEL
	par_sort(a, n, sizeof a[0], cmp);
#else
	qsort(a, n, sizeof a[0], cmp);
#endif

	end = sec();

	printf("%1.2f s\n", (end - start));
	for (i = 1; i < n; ++i)
	{
		if(a[i-1]>a[i])
			printf("err result\n");
	}

	free(a);

	return 0;
}
