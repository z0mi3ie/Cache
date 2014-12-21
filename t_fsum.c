//
// simple test of the simulated memory system
//

#include <stdio.h>
#include <stdlib.h>
#include "memSystem.h"

#define SIZE 16

int main(void){

  // create memory system with 256 words and a direct-mapped cache
  // with 8 sets and a block size of 4 words.
  void *a = initializeMemorySystem(SIZE, 1, 2, 2, 2);
  
  if (a == NULL){
    fprintf(stderr, "initializeMemorySystem failed!\n");
    exit(-1);
  }

  int i;
  float val, sum, sum2;
  sum2 = 0.0;

  // initialize the array
  for (i = 0; i < SIZE-3; i++){
    val = i*1.2;
    sum2 += val;
    writeFloat(a, 0, i, val);
  }

  // now sum it
  sum = 0.0;
  for (i = 0; i < SIZE-3; i++){
    sum += readFloat(a, 0, i);
  }

  printf("sum is %f (should be %f)\n", sum, sum2);

  // print stats
  printf("\n");
  printStatistics(a);
  printf("\n");

  return 0;
}


