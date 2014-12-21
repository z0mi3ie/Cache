//
// simple test of the simulated memory system
//

#include <stdio.h>
#include <stdlib.h>
#include "memSystem.h"

#define SIZE 256


//void printCache( void *h );

int main(void)
{
  // create memory system with 256 words and a direct-mapped cache
  // with 8 sets and a block size of 4 words.
  void *a = initializeMemorySystem(SIZE, 2, 4, 8, 4);
  //void *a = initializeMemorySystem(SIZE, 1, 4, 1, 1);
  if (a == NULL)
  {
    fprintf(stderr, "initializeMemorySystem failed!\n");
    exit(-1);
  }

  int i, sum, sum2;

  // initialize the array
  for (i = 0; i < SIZE; i++)
  {
    writeInt(a, 0, i, i);
  }

  //printCache( a );
  // now sum it
  sum = 0;
  sum2 = 0;
  for (i = 0; i < SIZE; i++)
  {
    int tmp = readInt(a, 0, i);
    // printf("# %d %d\n", tmp, i);
    sum += tmp;
    sum2 += i;
  }
  printf("sum is %d (should be %d)\n", sum, sum2);

                                                                                
  // print stats
  printf("\n");
  printStatistics(a);
  printf("\n");


  return 0;
}




// Save for LRU Replacement
/* 
int temp = 0;
fprintf( stderr, "===========================================================\n" );
for( i = 0; i < memsys->linesPerSet; i++ )
{
fprintf( stderr, " Line: %d LRU[ %d ]\n", i, memsys->cache[cache].set[sSet].line[li].lruStamp );
}
*/
