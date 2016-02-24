/*
Alex Daniels
CS 441
Assignment 3
assignment3.c

Pthreads implementation of Floyd-Warshall Algorithm
Compile as gcc -lpthread -o as3 assignment3.c
run as ./as3 [n] where n is number of threads

Runs Floyd-Warshall on n threads
Each thread does threadcount / matrixsize 

Each row of the distance matrix is accessed as an offset from 0
[i * matrixsize + j] === [i][j] in the loops
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/*
typedef struct 
{
	int *dist;				//distance matrix
	int threadCount;		//number of threads
	int matrixsize;			//distance matrix size
}ThreadStruct;
*/

int *dist;				//distance matrix
int threadCount;		//number of threads
int matrixsize;			//matrix size

pthread_barrier_t barrier;	//barrier

void *pathfinder(void *t);

int main(int argc, char** argv)
{
	char filename[32];
	FILE *file;
	int i, j;
	int retcode;
	long temp;
	//threads
	pthread_t *threads;
	pthread_attr_t attr;

	//make sure number of threads is passed as an argument
	if(argc < 2)
	{
		printf("Error number of threads not specified. Exiting...\n");
		exit(-1);
	}

	//get number of threads and allocate space for threads
	threadCount = atoi(argv[1]);
	threads = malloc(threadCount * sizeof(pthread_t));

	//get filename, check to make sure the file can be opened/exists, then open
	printf("Enter filename: ");
	scanf("%s", filename);
	if((file = fopen(filename, "r")) == NULL)
	{
		printf("Error opening file %s\n", filename);
		exit(-1);
	}	

	//get matrix size
	fscanf(file, "%d", &matrixsize);

	//allocate space for distance matrix
	dist = malloc(matrixsize * matrixsize * sizeof(int));

	//read in matrix
	for(i = 0; i < matrixsize; i++)
		for(j = 0; j < matrixsize; j++)
			fscanf(file, "%d", &dist[(i * matrixsize) + j]);
	fclose(file);

	//display matrix
	printf("%d x %d Cost Adjacency Matrix:\n", matrixsize, matrixsize);
	for(i = 0; i < matrixsize; i++)
	{
		for(j = 0; j < matrixsize; j++)
		{
			if(dist[i * matrixsize + j] == 1000000)
				printf("INF ");
			else
				printf("%d ", dist[(i * matrixsize) + j]);
		}
		printf("\n");
	}

	//initialize barrier object
	if(pthread_barrier_init(&barrier, NULL, threadCount))
	{
		printf("Error could not create barrier\n");
		exit(-1);
	}

	//init attribute object
	//set thread state to joinable
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	//create threads, send them to do work
	for(i = 0; i < threadCount; i++)
	{
		temp = (long) i;
		retcode = pthread_create(&threads[i], &attr, pathfinder, (void *) temp);
		if(retcode)
		{
			printf("Error could not create thread %d\n", i);
			exit(-1);
		}
	}

	//join threads
	for(i = 0; i < threadCount; i++)
	{
		if(pthread_join(threads[i], NULL))
		{
			printf("Error could not join thread\n");
			exit(-1);
		}
	}

	//free up space before exiting
	pthread_attr_destroy(&attr);
	pthread_barrier_destroy(&barrier);
	free(dist);
	pthread_exit(NULL);
}

void *pathfinder(void *t)
{
	int i, j, k;
	int retcode, distance;
  	int threadnum = (long) t;
  	int threadoffset1, threadoffset2;

	//block decomp offsets
	threadoffset1 = (threadnum * matrixsize) / threadCount; 			 //low end offset
	threadoffset2 = ((threadnum + 1) * matrixsize) / threadCount - 1;   //high end offset

	//wait for all threads to catch up
	retcode = pthread_barrier_wait(&barrier);
	if (retcode != 0 && retcode != PTHREAD_BARRIER_SERIAL_THREAD)
	{
		printf("Error could not wait on barrier\n");
		exit(-1);
  	}

  	//floyd-warshall
  	for (k = 0; k < matrixsize; k++)
  	{
		for (i = threadoffset1; i <= threadoffset2; i++) 
		{
      		for (j = 0; j < matrixsize; j++)
      		{
        		distance = dist[(i * matrixsize) + k ] + dist[(k * matrixsize) + j];
		        if(distance < dist[(i * matrixsize) + j])
		        	dist[(i * matrixsize) + j] = distance;
      		}
   		}
    	//wait for all threads to reach barrier
	    retcode = pthread_barrier_wait(&barrier);
	    if (retcode != 0 && retcode != PTHREAD_BARRIER_SERIAL_THREAD) 
	    {
	    	printf("Error could not wait on barrier\n");
	    	exit(-1);
	    }
	}	

	retcode = pthread_barrier_wait(&barrier);
    if (retcode != 0 && retcode != PTHREAD_BARRIER_SERIAL_THREAD) 
    {
    	printf("Error could not wait on barrier\n");
    	exit(-1);
    }
	if(retcode == PTHREAD_BARRIER_SERIAL_THREAD)
	{
		printf("Distance Matrix:\n");
		fflush(stdout);
	}
	retcode = pthread_barrier_wait(&barrier);
    if (retcode != 0 && retcode != PTHREAD_BARRIER_SERIAL_THREAD) 
    {
    	printf("Error could not wait on barrier\n");
    	exit(-1);
    }
	//print out revised matrix
	for(i = threadoffset1; i < threadoffset2; i++)
	{
		printf("Thread %d: ", threadnum);
		for(j = 0; j< matrixsize; j++)
			printf("%d ", dist[(i * matrixsize) + j]);
		printf("\n");
		fflush(stdout);
	}

	//and we're done here
	pthread_exit(NULL);
}
