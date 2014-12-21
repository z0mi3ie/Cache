//
// memory system simulation
//
// a memory system includes both memory and caches
//
// the memory consists of a sequence of 32-bit words
//    the length of memory and the number of caches are specified
//      at initialization
//
// each cache is an array of sets
// each set contains an array of lines
// each line contains a block plus control information
// each block is an array of words
//
// the caches for a memory system are all identical and can
// be configured during initialization:
//   the number of cache sets
//   the number of cache lines per set
//   the number of words per cache block
//
// all caches are write-back caches
//
// constraints:
//   the memory length must be a power of 2
//   the number of words per cache block must be power of 2
//   the number of cache sets must be a power of 2
//   the product of the number of words per cache block plus the number
//     of cache sets must be less than the memory length.

// initMemorySystem
//
// create a memory system, including a set of identical caches
// returns a "handle" for reading or writing to memory via a cache
//
// if the memory system cannot be created then NULL is returned
void *initializeMemorySystem(unsigned int memoryLength,
                       unsigned int numberOfCaches,
                       unsigned int wordsPerBlock, 
                       unsigned int numberOfSets,
                       unsigned int linesPerSet);

// readInt
//
// read an int from memory, checking the cache first
int readInt(void *handle, unsigned int cache, unsigned int address);

// readFloat
//
// read a float from memory, checking the cache first
float readFloat(void *handle, unsigned int cache, unsigned int address);

// writeInt
//
// write an int to memory, checking the cache first
void writeInt(void *h, unsigned int cache, unsigned int address,
  int value);

// writeFloat
//
// write a float to memory, checking the cache first
void writeFloat(void *h, unsigned int cache, unsigned int address,
  float value);

// printStatistics
//
// print the total number of reads, the number of reads that hit in the cache,
// the total number of writes, and the number of writes that hit in the cache.
void printStatistics(void *h);
