//
// simple test of the simulated memory system
//

#include <stdio.h>
#include <stdlib.h>
#include "memSystem.h"

#define SIZE 16

int main(void)
{
  // create memory system with 256 words and a direct-mapped cache
  // with 8 sets and a block size of 4 words.
  
  void *a = initializeMemorySystem(SIZE*SIZE, 1, 4, 8, 1);
  if (a == NULL)
  {
    fprintf(stderr, "initializeMemorySystem failed!\n");
    exit(-1);
  }

  // simulate two-dimensional array access in row-major order
  int i, j, sum;
  sum = 0;
  for (i = 0; i < SIZE; i++)
    for (j = 0; j < SIZE; j++)
      sum += readInt(a, 0, i*SIZE+j);

  // print stats
  printf("First row-major order:\n");
  printStatistics(a);
  printf("\n");

  // re-create the memory system
  a = initializeMemorySystem(SIZE*SIZE, 1, 4, 8, 1);
  if (a == NULL)
  {
    fprintf(stderr, "initializeMemorySystem failed!\n");
    exit(-1);
  }

  // simulate two-dimensional array access in column-major order
  sum = 0;
  for (i = 0; i < SIZE; i++)
    for (j = 0; j < SIZE; j++)
      sum += readInt(a, 0, j*SIZE+i);

  // print stats
  printf("Second column-major order:\n");
  printStatistics(a);
  printf("\n");

  return 0;
}


