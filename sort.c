#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/time.h>
#include <unistd.h>
#define N (2)
#define PARALLEL

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

void* work(void* arg){
	printf("hello \n");
	return NULL;
}

void par_sort(
	void*		base,	// Array to sort.
	size_t		n,	// Number of elements in base.
	size_t		s,	// Size of each element.
	int		(*cmp)(const void*, const void*)) // Behaves like strcmp
{

	


	// double p = ((double*)base)[0];
	// double t;
	// double* b1;
	// double* b2;
	// b1 = malloc(n * sizeof(double));
	// b2 = malloc(n * sizeof(double));
	// size_t n1 = 0;
	// size_t n2 = 0;
	// printf("err0 %1.0f\n",p);
	// for (int i = 0; i < n; ++i)
	// {
	// 	t = ((double*)base)[i];
	// 	printf("err2 %1.0f\n",t);
	// 	if(t>p){
	// 		b1[n1]=t;
	// 		n1++;
	// 	}else{
	// 		b2[n2]=t;
	// 		n2++;
	// 	}
	// }
	// printf("%1.0f %1.0f\n",b1[0],b2[0]);	
	// printf("%p %zu %zu %p\n",b1,n1,sizeof(double),cmp);
	pthread_t pt[N];
	if (pthread_create(&pt[0], NULL, work, NULL) != 0)
		printf("err c\n");
	if (pthread_create(&pt[1], NULL, work, NULL) != 0)
		printf("err c\n");
	// b2,n2,sizeof(double),cmp
	// for (int i = 0; i < N; ++i)
	// {
	// 	printf("err2 %d\n",i);
	// 	if(pthread_join(pt[i],NULL)!=0)
	// 		printf("err j\n");
	// }
	// free(b1);
	// free(b2);

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

	// for (int i = 1; i < 1000000000; ++i)
	// {
	// 	int d = 0;
	// 	d=19%i+i/31;
	// }

	end = sec();

	printf("%1.2f s\n", (end - start));
	printf("%1.2f %1.2f %1.2f %1.2f\n", a[0],a[1],a[2],a[3]);

	free(a);

	return 0;
}
