/** @file memory.c
 *  @brief Implements starting point for a memory hierarchy with caching and
 * RAM.
 *  @see memory.h
 */

#include "memory.h"

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

static unsigned long instr_count;

typedef enum{
  WRITE_THROUGH, //0
  WRITE_BACK, //1
} WritePolicies;

typedef enum{
  RANDOM, //0
  LRU, // 1
  TEMPORAL_SPATIAL, //2
}ReplacementPolicy;

typedef enum{
  NINE, // 0
  INCLUSIVE, //1
  EXCLUSIVE, // 2
}InclusivePolicy;

typedef struct { // size, associativity, line size, write back policy should be easily changeable.
  int size;
  int associativity;
  int inclusive;
  ReplacementPolicy replacement_policy;
  int line_width;
  int line_size;
  int bus_width;
  WritePolicies write_policy;
}Cache;

Cache *L1D;
Cache *L1I;
Cache *L2;

#define L1D_size 32000 //32kb
#define L1D_associativity 4
#define L1D_replacement_policy LRU
#define L1D_line_size 64
#define L1D_bus_width 32
#define L1D_write_policy WRITE_BACK

#define L1I_size 32000 // 32kb
#define L1I_associativity 8
#define L1I_replacement_policy RANDOM
#define L1I_line_size 64
#define L1I_bus_width 32
#define L1I_write_policy WRITE_BACK

#define L2_size 512000 //512kb
#define L2_associativity 8
#define L2_write_policy WRITE_BACK
#define L2_line_size 64
#define L2_replacement_policy RANDOM
#define L2_bus_width 32


void memory_init(void)
{
  L1D = (Cache*)malloc(sizeof(Cache));
  L1I = (Cache*)malloc(sizeof(Cache));
  L2 = (Cache*)malloc(sizeof(Cache));

  L1D->size = 0;
  L1D->associativity = 0;
  L1D->inclusive;
  L1D->replacement_policy;
  L1D->line_width;
  L1D->line_size;
  L1D->bus_width;
  L1D->write_policy;


  printf("initializing memory\n");
  
  /* Initialize memory subsystem here. */

  instr_count = 0;
}

void CacheLookup(Cache currentCache, uint64_t adress){

}

void CacheInsert(Cache currentCache, uint64_t adress){

}

void memory_fetch(uint64_t address, data_t *data)
{
/* PSEUDO
  CHECK IF L1I HITS OR MISSES
  if(cache_lookup(L1I, adress))
    L1I_hits++;
    else
      L1I_miss++;

  CHECK IF L2 HITS OR MISSES
  if(cache_lookup(L2,adress))
    L2_hits;
    cache_insert(L1I, adress)
    else
      L2_misses++;
      cache_insert(L2, adress)
      cachce_insert(L1I, adress)
*/
  printf("data = %p \n", (void*)data);
  printf("memory: fetch 0x%" PRIx64 "\n", address);
  if (data)
    *data = (data_t)0;

  instr_count++;
}

void memory_read(uint64_t address, data_t *data)
{
  /* PSEUDO
  CHECK IF L1I HITS OR MISSES
  if(cache_lookup(L1D, adress))
    L1I_hits++;
    else
      L1I_miss++;

  CHECK IF L2 HITS OR MISSES
  if(cache_lookup(L2,adress))
    L2_hits++;
    cache_insert(L1D, adress)
    else
      L2_misses++;
      cache_insert(L2, adress)
      cachce_insert(L1D, adress)
  
  
  */
  printf("memory: read 0x%" PRIx64 "\n", address);
  if (data)
    *data = (data_t)0;

  instr_count++;
}

void memory_write(uint64_t address, data_t *data)
{
  /*Write policies
  Write through : Data written to L1 has to then aswell be written to L2
  Write back: Data modified will be written to L1 and marked as dirty. When evicted from L1 the data
  is then written to the next level, L2 and again marked as dirty.
  
  */
  /*PSEUDO
  if(cache_lookup(L1D, adress))
    if(L1D->write_policy == write back)
      L1D_write_hits++;
      MarkDirty(L1D,adress);
    else
      L1_write_miss++;
  
  */
  printf("memory: write 0x%" PRIx64 "\n", address);

  instr_count++;
}

void memory_finish(void)
{
  fprintf(stdout, "Executed %lu instructions.\n\n", instr_count);

  /* Deinitialize memory subsystem here */
}


// int main(){
//   memory_init();
//   data_t testData = 2000;
//   memory_fetch(2, &testData);
//   printf("testing main\n");
// }