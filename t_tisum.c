//
// simple test of the simulated memory system
//

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "memSystem.h"

#define SIZE 16
#define SUMS_PER_THREAD 7
#define SUM_MAX 14
#define THREADS 2
#define BLOCKS 2
#define SETS 2
#define LINES 2
//
// Cache reads/writes: 2*SUMS_PER_THREAD
// Cache hits on read/write: Undefined??

void *sys;
pthread_t pt[THREADS];
int threads_exec = 0;

pthread_mutex_t mu;
pthread_cond_t cv;

void * work(void *in){
  int cacheNum =  (int) in;

  // Fill our cache with our inputs
  int val, addr, tSum;
  for(int i=0; i<SUMS_PER_THREAD; i++){
    val = (SUMS_PER_THREAD*cacheNum)+i;
    addr = val + 1;

    writeInt( sys, cacheNum, addr, val );
  }

  // Read our inputs back out, stuff the sum into
  // memory[0] (aka, the global sum).
  for(int i=0; i<SUMS_PER_THREAD; i++){
    addr = (SUMS_PER_THREAD*cacheNum)+i+1;
    val = readInt( sys, cacheNum, addr );

    tSum = readInt( sys, cacheNum, 0 );
    tSum += val;
    writeInt( sys, cacheNum, 0, tSum );
  }

  if (pthread_mutex_lock(&mu) != 0)
    perror("error in mutex_lock in slave");

  if (++threads_exec >= THREADS){
    if (pthread_cond_signal(&cv) != 0)  /* wakeup master */
      perror("error in cond_signal");
  }

  if (pthread_mutex_unlock(&mu) != 0)
    perror("error in mutex_unlock in slave");

  return NULL;
}

int main(void){

  // Single threaded true result, for testing.
  int trueSum = 0;
  for(int i=0; i<SUM_MAX; i++){
    trueSum += i;
  }

  /*
  // Initial Output, for debugging and such.
  printf("--------------------------------------------\n");
  printf("| Running the Integer Sum (Multi-threaded) |\n");
  printf("--------------------------------------------\n");
  printf("|   @ Sum numbers 1-N:         | %9d |\n", SUM_MAX );
  printf("|   @ Sums Handled Per Thread: | %9d |\n", SUMS_PER_THREAD );
  printf("|   @ Memory Size:             | %9d |\n", SIZE );
  printf("|   @ Threads Executing:       | %9d |\n", THREADS );
  printf("|   @ Blocks Per Line:         | %9d |\n", BLOCKS );
  printf("|   @ Lines Per Set:           | %9d |\n", LINES );
  printf("|   @ Sets Per Cache:          | %9d |\n", SETS );
  printf("--------------------------------------------\n");
  printf("|   @ Correct Total Reads:     | %9d |\n", (2*SUMS_PER_THREAD) );
  printf("|   @ Correct Read Hits:       |  [Varies] |\n");
  printf("|   @ Correct Total Writes:    | %9d |\n", (2*SUMS_PER_THREAD) );
  printf("|   @ Correct Write Hits:      |  [Varies] |\n");
  printf("|   @ Correct Answer:          | %9d |\n", trueSum );
  printf("--------------------------------------------\n");
  printf("| Note: Total Reads Will Be +1 due to sum  |\n");
  printf("| calculation at end of file. This is OK.  |\n");
  printf("--------------------------------------------\n");
  */

  // Initialize Memory System.
  sys = initializeMemorySystem( SIZE, THREADS, BLOCKS, SETS, LINES );
  if (sys == NULL){
    fprintf(stderr, "initializeMemorySystem failed!\n");
    exit(-1);
  }

  // Init Mutexes
  if (pthread_mutex_init(&mu, NULL) != 0)
    perror("can't init mutex");
  if (pthread_cond_init(&cv, NULL) != 0)
    perror("can't init condition variable");

  // Create the threads with our supplied work function.
  for (int i=0; i < THREADS; i++){
    if( pthread_create( &pt[i], NULL, work, (void*) i) )
          perror("error in thread create");
  }

  // lock threads, see if everyone is finished, if they are, keep the llock 
  // until execution is about to finish.
  if (pthread_mutex_lock(&mu) != 0)
    perror("error in mutex_lock in slave");

  if ( threads_exec < THREADS){
    if (pthread_cond_wait(&cv, &mu) != 0) /* wait for all slaves to finish */
         perror("error in cond_wait by master");
  }
  

  // memory[0] is reserved for our global sum (to create competition).
  int calc = readInt( sys, 0, 0 );
  printf("=> %d [actual: %d]\n\t%s \n",calc, trueSum, (calc==trueSum)?"CORRECT!":"**FAILURE**" );
  
  // release mutex before termination.
  if (pthread_mutex_unlock(&mu) != 0)
    perror("error in mutex_unlock in slave");

  return 0;
}
