////////////////////////////////////////
//
//
// Kyle Vickers
//
// Progam 6
//
// Memory Sub System
//
// This is a simulation of a memory subsystem.
// It supports more than one cache, it is a 
// writeback system ( not a write through )
// and supports single directly mapped memory
// management as well as set associative caches
//
//
////////////////////////////


#include <stdlib.h>
#include <stdio.h>
#include "memSystem.h"


////////////////////////////////
////////// MY DEFINES //////////
#define SUCCESS  1            //
#define FAILURE  0            // 
#define TRUE     1            // 
#define FALSE    0            //
#define FULLBITS 32           //
#define FULLMASK 0xFFFFFFFF   //
#define UNSINT   unsigned int //
#define HIT      0
#define MISS     1

  //
 // STATICS 
/////////////////////////////////////////////////
static void testAddress( void *h, unsigned int, unsigned int );
static int powerCheck( UNSINT  in              );
static UNSINT maskFactory( UNSINT  a, UNSINT b     );
static int computeLog2( UNSINT  in              );
static void tripleSlice( UNSINT, UNSINT, UNSINT, 
                           UNSINT*,UNSINT*,UNSINT* );

void printCache( void *h );
void printUsedBlocks( void *h);

////////////////////////////////////////////////
////// MEMORY SYSTEM DATA STRUCTURE ////////////
typedef struct Line {
  int rawAdd;
  unsigned int tag;
  int full;
  int valid;
  int dirty;
  int lruStamp;
  int usedBlockCount;
  int * block;
} LINE;


typedef struct Set {
  LINE * line;
} SET;


typedef struct Cache{
  SET * set;
  int totalReads;
  int totalReadHits;
  int totalWrites;
  int totalWriteHits;
  int clock;
} CACHE;


typedef struct MemorySystem{ 
  int * pMemory;
  CACHE * cache;
  int memoryLength;
  int numberOfCaches;
  int wordsPerBlock;
  int numberOfSets;
  int linesPerSet;
  int offsetBits;
  int setBits;

} MEMSYS;

 
//////////////////////////////////////////////////////////////////////
/*                                                                 //
 * static int powerCheck( int )                                   //
 * Returns SUCCESS if power of 2
 * Returns FAILURE if not power of 2
 *
 */
static int powerCheck( unsigned int in )
{ 
  if( ( ( in != 0 ) && ( in & (in - 1 ) ) ) == 0 )
    return SUCCESS;
    
  return FAILURE;
}

/////////////////////////////////////////////////////////////////////////
/*                                                                    // 
 * int computeLog2( unsigned int )                                   // 
 * This function computes the integer log2 of the 
 * unsigned int passed to it and returns it as an int. 
 *
 */
static int computeLog2( unsigned int in ) {
  
  int ret = 0;
  while( in >>= 1 ) {
    ret++;
  }
  
  return ret;
}


//////////////////////////////////////////////////////////////////////////
/*                                                                     //
 * void tripleSlice( ''' BY VAL ''' numOffBits, numSetBits, address   // 
 *                   ''' BY REF ''' off, set, tag 
 *
 * This function takes an address and splits it up into three separate
 * unsigned integers (UNSINT) to be used for further application in the
 * the Cache system. 
 *
 */
static void tripleSlice( UNSINT numOffBits, UNSINT numSetBits, 
                                              UNSINT address,
            /*To save: *off = _val_*/         UNSINT * off, 
            /*To save: *set = _val_*/         UNSINT * set,   
            /*To save: *tag = _val_*/         UNSINT * tag ) {
  
  UNSINT mask; 
  
  mask = maskFactory( 0, numOffBits ); 
  *off = ( address & mask );

  mask = maskFactory( numOffBits, numOffBits + numSetBits  );  
  *set = ( address & mask );
  *set >>= numOffBits;

  mask = maskFactory( numOffBits + numSetBits, FULLBITS );  
  *tag = ( address & mask );
  *tag >>= ( numOffBits + numSetBits ); 
}


//////////////////////////////////////////////////////////////////////
/* maskFactory( unsigned, unsigned )                               //
 *                                                                // 
 * This function returns a bitwise mask given a low bit
 * and a high bit. 
 * 
 */
static unsigned int maskFactory( UNSINT lowBit, UNSINT highBit ) {
  int i = lowBit;
  UNSINT out = 0;
  
  while( i < highBit )
  {
    out |= 1 << i;
    i++;
  }
  return out;
}


//////////////////////////////////////////////////////////////////
/*                                                             //
 *  Checks the specified cache to see if the word is present. //
 *
 *  This function DOES NOT take care of hits or misses, we will 
 *  take care of that back in the other function for writeInt.
 *
 *  This function DOES NOT modify the MEMORY SYSTEM at ALL
 */
static int checkcache( void*h, const UNSINT cache, const UNSINT offset, 
                        const UNSINT setIndex, const UNSINT tag ) {
  
  MEMSYS * memsys = (MEMSYS*)h;
  int i, numLines = memsys->linesPerSet;
  
  for( i = 0; i < numLines; i++ ) { 
    if( memsys->cache[cache].set[setIndex].line[i].tag == tag ) 
      return SUCCESS;
  }

  return FAILURE;
}

////////////////////////////////////////////////////////////////////
/*                                                               //
 * checkLine returns a pointer to a line in the cache system.   // 
 * It returns null if it is a a failure ( there is not matching
 * Line in the set. 
 */
static LINE * checkLine( void*h, UNSINT address, UNSINT cache, UNSINT * turnout ) { 
  MEMSYS * memsys = (MEMSYS*)h;
  int i, j, k, l;

  // Make it null if failure //
  UNSINT sOff      = 0;                        
  UNSINT sSet      = 0;                          
  UNSINT sTag      = 0;                          
  *turnout = FALSE;                                                    
  tripleSlice( memsys->offsetBits, memsys->setBits, address,              
                                &sOff, &sSet, &sTag           ); 
  
  for( i = 0; i < memsys->linesPerSet; i++ ) {
    if( memsys->cache[cache].set[sSet].line[i].tag == sTag ){
      *turnout = HIT;
      return &memsys->cache[cache].set[sSet].line[i]; // Found a matching tag
    }
  }


  //Check other caches for a match
  //Only used for when there is more than one cache
  if( memsys->numberOfCaches > 1 ) {
    for( i=0; i < memsys->numberOfCaches; i++ ) {
      if( i != cache && checkcache( h, i, sOff, sSet, sTag ) ) { 
        for( j=0; j < memsys->numberOfSets; j++ ) { 
          for( k=0; k < memsys->linesPerSet; k++ ) {
            // WRITE BACK //
            if( memsys->cache[i].set[j].line[k].tag == sTag ) { 
              UNSINT address = memsys->cache[i].set[j].line[k].rawAdd;
              for( l=0; l < memsys->wordsPerBlock; l++ ) { 
                memsys->pMemory[address+l] = 
                       memsys->cache[i].set[j].line[k].block[l];
              
                // CLEAR OUT THE LINE //
                memsys->cache[i].set[j].line[k].block[l] = 0; 
                memsys->cache[i].set[j].line[k].dirty    = 0;
                memsys->cache[i].set[j].line[k].valid    = 0;
                memsys->cache[i].set[j].line[k].tag      = 0;
              }
            }
          }
        }
      } 
    }
  }
 
  /////////////////////////////////
  ////// INVALID LINE CHECK ///////
  for( i = 0; i < memsys->linesPerSet; i++ ) {
    if( memsys->cache[cache].set[sSet].line[i].valid == FALSE ){
        *turnout = MISS;
        return &memsys->cache[cache].set[sSet].line[i]; 
    }
  }
  
  //////////////
  //CHECK LRU //
  UNSINT tmp = 0xFFFFFFFF; 
  int lowest; 
  for( i=0; i<memsys->linesPerSet; i++ ) {
    if( tmp > memsys->cache[cache].set[sSet].line[i].lruStamp ) { 
      tmp = memsys->cache[cache].set[sSet].line[i].lruStamp;
      lowest = i;
    }
  }
  
  ///////////////////////////////////////////////////
  //IF THE DIRTY BIT IS ON THEN LOAD CACHE TO MEMORY
  if( memsys->cache[cache].set[sSet].line[lowest].dirty ) {      
     UNSINT address = memsys->cache[cache].set[sSet].line[lowest].rawAdd;
     for( i=0; i < memsys->wordsPerBlock; i++ ) 
         memsys->pMemory[address+i] = 
          memsys->cache[cache].set[sSet].line[lowest].block[i];
  }


  //////////////////////////////////////////
  // CLEAR OUT THE LINE  //////////////////
  for( i=0; i < memsys->wordsPerBlock; i++ ) 
    memsys->cache[cache].set[sSet].line[lowest].block[i] = 0;

  memsys->cache[cache].set[sSet].line[lowest].dirty = 0;
  memsys->cache[cache].set[sSet].line[lowest].valid = 0;
  memsys->cache[cache].set[sSet].line[lowest].tag = 0;
  
  return &memsys->cache[cache].set[sSet].line[lowest]; 
}


//////////////////////////////////////////////////////////////////////
/*                                                                 
 * void * initializeMemorySystem                                   
 *
 * Creates a memory system to be used with a cache, and the
 * cache itself. 
 *
 * A cash is constructed as:
 *
 * Cash->Set->Line->Block
 *
 * The memory system holds the cache and is returned in a 
 * void* handle
 *
 */
void * initializeMemorySystem(UNSINT memoryLength, UNSINT numberOfCaches, 
      UNSINT wordsPerBlock, UNSINT numberOfSets, UNSINT linesPerSet     ) {

  if( powerCheck(memoryLength) == FAILURE ) {
    fprintf( stderr, "initializeMemory failed powerCheck(memoryLength)\n" );
    return NULL;
  }
 
  if( powerCheck(wordsPerBlock) == FAILURE ) {
    fprintf( stderr, "initializeMemory failed powerCheck(wordsPerBlock)\n" );
    return NULL;
  }
  
  if( powerCheck(numberOfSets) == FAILURE )  {
    fprintf( stderr, "initializeMemory failed powerCheck(numberOfSets)\n" );
    return NULL;
  }

  if( wordsPerBlock * numberOfSets >= memoryLength ) {
    fprintf( stderr, "initializeMemory failed words * sets >= length \n" );
    return NULL;
  }
 
  int i,j,k;
 
  //////////////////////////////////////////
  //// SET UP MEMORY SYSTEM VARS        ////
  MEMSYS * memsys = malloc( sizeof(MEMSYS) ); 
  if( !memsys ) {
    fprintf(stderr, "memsys failed to initialize FATAL\n");
    exit(-1);
  }
  
  memsys->pMemory = malloc( sizeof(int) * memoryLength ); 
  if( !memsys->pMemory ) {
    fprintf( stderr, "pMemory failed to initialize FATAL\n" );
    exit(-1);
  }
  
  memsys->cache   = malloc( sizeof(CACHE) * numberOfCaches );
  if( !memsys->cache ) {
    fprintf( stderr, "cache failed to initialize FATAL\n" );
    exit(-1);
  }
  
  memsys->memoryLength   = memoryLength;
  memsys->numberOfCaches = numberOfCaches;
  memsys->wordsPerBlock  = wordsPerBlock;
  memsys->numberOfSets   = numberOfSets;
  memsys->linesPerSet    = linesPerSet;
  memsys->offsetBits     = computeLog2( wordsPerBlock );
  memsys->setBits        = computeLog2( numberOfSets );
  
  ///////////////////////////////////////
  ////  SETUP REST OF DATA STRUCT    ////
  for( i = 0; i < numberOfCaches; i++ ){ 
    memsys->cache[i].set = malloc( sizeof( SET ) * numberOfSets );
    if( !memsys->cache[i].set ) {
      fprintf( stderr, "malloc fail [set]: returned FAILURE \n" );
      return NULL;
    }

    //////////////////////////////
    /// SETUP CACHE INFO STATS ///
    memsys->cache[i].totalReads       = 0;
    memsys->cache[i].totalReadHits    = 0;
    memsys->cache[i].totalWrites      = 0; 
    memsys->cache[i].totalWriteHits   = 0;
    memsys->cache[i].clock            = 0; 

    for( j = 0; j < numberOfSets; j++ ) {
      memsys->cache[i].set[j].line = malloc( sizeof( LINE ) * linesPerSet ); 
      if( !memsys->cache[i].set[j].line ) {
        fprintf( stderr, "malloc fail [line]: returned FAILURE \n" );
        return NULL;
      }

      for( k = 0; k < linesPerSet; k++ ) {
       
       memsys->cache[i].set[j].line[k].block = 
                          malloc( sizeof(int) * wordsPerBlock );
        
        if( !memsys->cache[i].set[j].line[k].block ) {
          fprintf( stderr, "malloc fail [block]: returned FAILURE \n" );
          return NULL;
        }

        ///////////////////////////////////
        ////   LINE CONTROL INFO INIT  ////
        memsys->cache[i].set[j].line[k].valid     = FALSE;
        memsys->cache[i].set[j].line[k].full      = FALSE; 
        memsys->cache[i].set[j].line[k].dirty     = FALSE;
        memsys->cache[i].set[j].line[k].tag       = -1;
        memsys->cache[i].set[j].line[k].lruStamp  = 0;
        memsys->cache[i].set[j].line[k].usedBlockCount = 0;
      }
    }
  }
  
  ///////////////////////////////////
  ////    RETURN MEMSYS HANDLE   ////
  return memsys;
}

//////////////////////////////////////////////////////////////////////
/*                                                                 //
 * int readInt( void *handle, UNSINT cache, UNSINT address        // 
 * This function reads an int from the cache given the
 * specified cache number and specified address the user 
 * would like to put this information into the memory. 
 *
 */
int readInt(void *handle, unsigned int cache, unsigned int address)
{ 
  MEMSYS * memsys = (MEMSYS*)handle;
  testAddress( handle, address, cache );

  int i;

  UNSINT sOff   = 0;                        
  UNSINT sSet   = 0;                          
  UNSINT sTag   = 0;                          
                                                      
  tripleSlice(  memsys->offsetBits, memsys->setBits, address,              
                                &sOff, &sSet, &sTag           ); 
                                                      
  unsigned int turnout = 0;
  LINE *line = checkLine( handle, address, cache, &turnout );

  if( line->valid ) {
    // Word was found in the cache  
    memsys->cache[cache].totalReads++;
    memsys->cache[cache].clock++;
    memsys->cache[cache].totalReadHits++;
    return line->block[sOff];
    
  }
  else {
    int botMemBlock = 0;   
    for( i = 0; i < memsys->memoryLength; i+= memsys->wordsPerBlock ) {
      if(( address >= i) && address < ( i + memsys->wordsPerBlock )){
        botMemBlock = i;
      } 
    }

    for( i = 0; i < memsys->wordsPerBlock; i++ )
        line->block[i] = memsys->pMemory[botMemBlock + i]; 

    memsys->cache[cache].totalReads++;
    memsys->cache[cache].clock++;
    line->rawAdd = botMemBlock;
    line->valid = TRUE;
    line->dirty = FALSE;
    line->usedBlockCount = 0;
    line->lruStamp = memsys->cache[cache].clock;
    line->tag = sTag;
  }

  return line->block[sOff]; 
}


//////////////////////////////////////////////////////////////////////
/*                                                                 //
 * readFloat( void*, unsigned int, unsigned int )                 //
 * 
 * This function reads a floating point number from memory and
 * returns in to the the user. It uses pointers and casting to
 * get the bits out of the integer array by calling readInt
 * with the bits from the float 
 *
 */
float readFloat(void *handle, unsigned int cache, unsigned int address) {

  int intReturned = readInt( handle, cache, address );
  return *(float*)&intReturned; 
}



//////////////////////////////////////////////////////////////////////
/*                                                                 //
 * void writeInt( void*, UNSINT, UNSINT, int )                    //
 *
 * This functiom writes an int to the cache and then the memory
 * depending on the situation. It takes a cache, address, and
 * a value and with those it is able to figure out what block
 * in the cache should receive the value
 */
void writeInt(void *h, unsigned int cache, unsigned int address, int value) {
 
  MEMSYS * memsys = (MEMSYS*)h;
  testAddress( h, address, cache );
  int i;

  UNSINT sOff      = 0;                        
  UNSINT sSet      = 0;                          
  UNSINT sTag      = 0;                          
                                                      
  tripleSlice(memsys->offsetBits, memsys->setBits, address, &sOff, &sSet, &sTag); 
  
  UNSINT turnout = 0; 
  LINE * li = checkLine( h, address, cache, &turnout );

  if( !li->valid ) {
    //1) Load blocks into cache
    int botMemBlock = 0;   
    for( i = 0; i < memsys->memoryLength; i+= memsys->wordsPerBlock ) {
      if(( address >= i) && address < ( i + memsys->wordsPerBlock ))
        botMemBlock = i;
    }
    
    for( i = 0; i < memsys->wordsPerBlock; i++ ) {
      li->block[i] = memsys->pMemory[botMemBlock + i]; 
    }
  
    li->rawAdd = botMemBlock;  
    li->block[sOff] = value;  
    li->valid = TRUE;
    li->dirty = TRUE;
    li->lruStamp  = memsys->cache[cache].clock;
    li->tag = sTag; 

  }
  else  {
    li->block[sOff] = value;
    li->dirty = TRUE;
    memsys->cache[cache].totalWriteHits++;    
  }  
  
  memsys->cache[cache].totalWrites++; 
  memsys->cache[cache].clock++; 
} 

//////////////////////////////////////////////////////////////////////
/* writeFloat( void*, UNSINT, UNSINT, value                        //
 *                                                                // 
 * This function writes a float to an integer based array.
 * It does this by sending a converted copy of its bits to
 * The writeFloat function.
 * * */ 
void writeFloat(void *h, unsigned int cache, unsigned int address, float value) {
    writeInt( h, cache, address, *( int *)&value ); 
} 

//////////////////////////////////////////////////////////////////////
/*                                                                 //
 * printStatistics( void* )                                       // 
 *
 * Given a handle for a cache structure this function will print
 * out the total number of: 
 *  reads, read hits, writes, and write hits
 * As well as what cash each of these performed in
 *
 */
void printStatistics(void *h)
{
  MEMSYS * memsys = (MEMSYS*)h;
  int i;
  for( i = 0; i < memsys->numberOfCaches; i++ )  {
    fprintf( stdout, "Cache %d:\n", i );

    fprintf( stdout, "  total number of reads: %d\n", 
                                           memsys->cache[i].totalReads );
    fprintf( stdout, "  reads that hit in cache: %d\n",
                                           memsys->cache[i].totalReadHits );
    fprintf( stdout, "  total number of writes: %d\n", 
                                           memsys->cache[i].totalWrites );
    fprintf( stdout, "  writes that hit in cache: %d\n", 
                                           memsys->cache[i].totalWriteHits );
  }
}


//////////////////////////////////////////////////////////////////////
/*                                                                 //
 *                                                                //
 * testAddress( void*, unsigned int, unsigned int )
 *
 * This function tests the address of all parameters passed to it
 * It tests for:
 *    cache index
 *    address size
 *    offset index
 *    line index
 */
static void testAddress( void* h, unsigned int add, unsigned int cache ) {
  MEMSYS * mem = (MEMSYS*)h;

  UNSINT sOff   = 0;                        
  UNSINT sSet   = 0;                          
  UNSINT sTag   = 0;                          
                                                      
  tripleSlice(  mem->offsetBits, mem->setBits, add,              
                                &sOff, &sSet, &sTag           ); 
  
  if( add >= mem->memoryLength ) {
    fprintf( stderr, "Address is greater than memory length FATAL\n" );
    exit(-1);
  }

  if( sOff >= mem->wordsPerBlock ) {
    fprintf( stderr, "Offset too big for cache :: FATAL\n" );
    exit(-1);
  }
  
  if( sSet >= mem->numberOfSets ) {
    fprintf( stderr, "Set index too big for cache :: FATAL\n" );
    exit(-1);
  }
  
  if( cache >= mem->numberOfCaches ) {
    fprintf( stderr, "Cache index too big for cache :: FATAL\n" );
    exit(-1);
  }
}
