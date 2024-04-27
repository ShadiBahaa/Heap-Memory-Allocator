#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#define _BSD_SOURCE

int min(int a, int b){
    if (a < b)return a;
    return b;
}

#define MAX_ALLOCS 10000
int
main(int argc, char *argv[])
{
    srand(time(NULL));   
    char *ptr[MAX_ALLOCS];
    int freeStep, freeMin, freeMax, blockSize, numAllocs, j;

    printf("\n");

    numAllocs = rand() % MAX_ALLOCS + 1;

    blockSize = rand() % MAX_ALLOCS + 1;

    freeStep = rand() % MAX_ALLOCS + 1;
    freeMin =  rand() % MAX_ALLOCS  + 1;
    freeMax =   min(numAllocs, rand()%MAX_ALLOCS + 1);

    if (freeMax > numAllocs)
        printf("free-max > num-allocs\n");

    printf("Initial program break:          %10p\n", sbrk(0));

    printf("Allocating %d*%d bytes\n", numAllocs, blockSize);
    for (j = 0; j < numAllocs; j++) {
        ptr[j] = malloc(blockSize);
        if (ptr[j] == NULL)
            printf("malloc\n");
    }

    printf("Program break is now:           %10p\n", sbrk(0));

    printf("Freeing blocks from %d to %d in steps of %d\n",
                freeMin, freeMax, freeStep);
    for (j = freeMin - 1; j < freeMax; j += freeStep)
        free(ptr[j]);

    printf("After free(), program break is: %10p\n", sbrk(0));

    exit(EXIT_SUCCESS);
}