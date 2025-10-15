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
#include <math.h>

static unsigned long instr_count;

typedef enum{
  WRITE_THROUGH, //0
  WRITE_BACK, //1
} WritePolicies;

typedef enum{
  RANDOM, //0 - start with using random as that seems easiest to impliment, then expand
  LRU, // 1 - least recently used Is the block that is furthest from recently used blocks
  TEMPORAL_SPATIAL, //2
}ReplacementPolicy;

typedef enum{
  NINE, // 0
  INCLUSIVE, //1
  EXCLUSIVE, // 2
}InclusivePolicy;

typedef enum{ // viktig punkt å vite. Les opp på associativity, forstå det skikkelig.
  DIRECT_MAPPING,
  ASSOCIATIVE_MAPPING,
  SET_ASSOCIATIVE_MAPPING,
}Associativity;

typedef struct // Defines what each line holds. A single cache entry
{
  int valid;
  int tag;
}CacheLine;

typedef struct // defines the "blocks". Represented associative amount of lines
{
  CacheLine *lines; 
}CacheSet;

typedef struct { // size, associativity, line size, write back policy should be easily changeable.
  int size; // number of sets = cache size / (line_size * associativity) //  depends on the associativity
  int associativity;
  int inclusive;
  ReplacementPolicy replacement_policy;
  int line_width;
  int line_size;
  int bus_width;
  int hit;
  int miss;
  int amount_sets;
  CacheSet *sets;
  WritePolicies write_policy;
}Cache;





Cache *L1D;
Cache *L1I;
Cache *L2;
// ---------- Verdier må etterhvert endres til realistiske verdier, sjekk readme -------------- //
// Dette vil bli bestemt av størrelsen på loggfilen. Antar at l1 skal være betydelig mindre enn fil størrelse
#define L1D_size 32000 //32kb
#define L1D_associativity 1 // set to 1 for testing direct mapping
#define L1D_replacement_policy RANDOM
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

int calculate_number_sets(Cache *currentCache){
  int result = 0;
  if(currentCache->associativity == DIRECT_MAPPING){
    result = currentCache->line_size;
    return result;
  }
}

void memory_init(void)
{
  L1D = (Cache*)malloc(sizeof(Cache));
  L1I = (Cache*)malloc(sizeof(Cache));
  L2 = (Cache*)malloc(sizeof(Cache));

  L1D->size = 0;
  L1D->associativity = 0;
  L1D->inclusive;
  L1D->replacement_policy = L1D_replacement_policy;
  L1D->line_size = L1D_line_size;
  L1D->bus_width = L1I_bus_width;
  L1D->write_policy = L1D_write_policy;
  L1D->miss = 0;
  L1D->amount_sets = calculate_number_sets(L1D);


  printf("initializing memory\n");
  

  instr_count = 0;
}

int CacheLookup(Cache *currentCache, uint64_t adress){
  /*
  For finding LSB(least significant bits) use this formula:
  masked_bits = ((1<<N)-1)
  This will give the N amount of bits as 1's so that together with original adress will output
  only the N amount of LSB.

  masked_bits & original adress = LSB


  For extracting for example bit at position 5 to position 6 from the right lets visualize:
  Original bits 0000 1011 1100
  bit adress start at position 0 -> 11
  If we want to extract bit 11 at position 5 and 6 then we first shift the bits that we dont want out with:
  shifted_adress =  adress >> 5  // this gives us 0000 0000 0101
  masked_adress = ((1<<2)-1) // this gives us 0000 0000 0011
  extracted_2_LSB = masked_adress & shifted_adress // this gives us 0000 0000 0001
  which then is the extracted bits at location 5 and 6.
  */
  int offsetBits = log2(currentCache->line_size); // expected value 6 from line_size = 64
  printf("offset bit = %d\n", offsetBits);
  int index_bits = log2(currentCache->amount_sets); //expected value 6 from sets = linesize = 64

  return 0; // returns 0 for no found adress with valid bit 1, can changed to returning the valid bit
}

void CacheInsert(Cache currentCache, uint64_t adress){

}

void memory_fetch(uint64_t address, data_t *data)
{
  CacheLookup(L1D, address);
/* PSEUDO
  CHECK IF L1I HITS OR MISSES
  if(cache_lookup(L1I, adress) && cache_tag == valid)
    L1I->hits++;
    else
      L1I->miss++;

  CHECK IF L2 HITS OR MISSES
  if(cache_lookup(L2,adress)&& cache_tag == valid)
    L2->hits++;
    cache->insert(L1I, adress)
    else
      L2->misses++;
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
  if(cache_lookup(L1D, adress)&& cache_tag == valid)
    L1I->hits++;
    else
      L1I->miss++;

  CHECK IF L2 HITS OR MISSES
  if(cache_lookup(L2,adress) && cache_tag == valid)
    L2->hits++;
    cache_insert(L1D, adress)
    else
      L2->misses++;
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