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
#include <time.h>

static unsigned long instr_count;

typedef enum // enum for write policy for easy readability 
{
  WRITE_THROUGH,
  WRITE_BACK,
} WritePolicies;

typedef enum //enum for replacement policy for easy readability 
{
  RANDOM,      
  LRU,     
  TEMPORAL_SPATIAL, 
} ReplacementPolicy;

typedef enum //enum for the different associativities for easy readability
{ 
  DIRECT_MAPPING,
  ASSOCIATIVE_MAPPING,
  SET_ASSOCIATIVE_MAPPING,
} Associativity;

typedef enum // Dirty enum to have a easy readable way of handling dirty and not dirty
{
  NOT_DIRTY, 
  DIRTY,   
} Dirty;

typedef struct // structure for each line 
{
  int valid;
  uint64_t tag;
  int markDirty;
} CacheLine;

typedef struct //structure for each set
{
  CacheLine *lines;
} CacheSet;

typedef struct // structure of the different parts of a adress for easier handling when returning tag, index, offset from a function
{
  uint64_t tag;
  uint64_t indexx; // Intentionally mispelled, index is a legacy function
  uint64_t offset;
} AdressParts;

typedef struct // structure of the hitmiss counters used in the cache
{
  int read_hit;
  int read_miss;
  int write_hit;
  int write_miss;
} Hit_Miss;

typedef struct // Structure of a cache
{ 
  int size;
  int associativity;
  int mapping;
  int inclusive;
  ReplacementPolicy replacement_policy;
  int line_width;
  int line_size;
  int bus_width;
  Hit_Miss hit_miss;
  int amount_sets;
  CacheSet *sets;
  WritePolicies write_policy;
} Cache;

Cache *L1D;
Cache *L1I;
Cache *L2;

// --------------------------- Changeable configurations to optimize the cache ------------------- //
#define L1I_size 512 
#define L1I_associativity 2
#define L1I_mapping SET_ASSOCIATIVE_MAPPING 
#define L1I_replacement_policy LRU
#define L1I_line_size 64
#define L1I_bus_width 64
#define L1I_write_policy WRITE_BACK

#define L1D_size 512                
#define L1D_associativity 2       
#define L1D_mapping SET_ASSOCIATIVE_MAPPING 
#define L1D_replacement_policy LRU
#define L1D_line_size 64
#define L1D_bus_width 64 
#define L1D_write_policy WRITE_BACK

#define L2_size 1024 // 4kb
#define L2_associativity 2
#define L2_mapping SET_ASSOCIATIVE_MAPPING
#define L2_replacement_policy LRU
#define L2_line_size 64
#define L2_bus_width 64
#define L2_write_policy WRITE_BACK

int calculate_number_sets(Cache *currentCache)
// Calculates the number of sets and returns the amount.
{
  int result = 0;

  result = currentCache->size / (currentCache->line_size * currentCache->associativity);
  return result;
}

void allocateDirectMapped(Cache *currentCache)
//Function for allocating memory for a direct mapped cache
{
  for (size_t i = 0; i < currentCache->amount_sets; i++)
  {
    currentCache->sets[i].lines = malloc(sizeof(CacheLine) * currentCache->associativity);
    for (size_t j = 0; j < currentCache->associativity; j++)
    {
      currentCache->sets[i].lines[j].valid = 0;
      currentCache->sets[i].lines[j].tag = 0;
      currentCache->sets[i].lines[j].markDirty = NOT_DIRTY;
    }
  }
}

void allocateSetAssosiativeMapped(Cache *currentCache)
//Allocate memory for a set associative mapped cache
{
  for (size_t i = 0; i < currentCache->amount_sets; i++)
  {
    currentCache->sets[i].lines = malloc(sizeof(CacheLine) * currentCache->associativity);
    for (size_t j = 0; j < currentCache->associativity; j++)
    {
      currentCache->sets[i].lines[j].valid = 0;
      currentCache->sets[i].lines[j].tag = 0;
      currentCache->sets[i].lines[j].markDirty = NOT_DIRTY;
    }
  }
}

Cache *cache_initialization(
    Cache *currentCache,
    int size, Associativity associativity,
    int mapping, int replacementPolicy,
    int line_size, int bus_width,
    int write_policy)
    //Initializes a cache, dynamically handled using the cache and values sent in as arguments
{

  currentCache->size = size;
  currentCache->associativity = associativity;
  currentCache->mapping = mapping;
  currentCache->replacement_policy = replacementPolicy;
  currentCache->line_size = line_size;
  currentCache->bus_width = bus_width;
  currentCache->write_policy = write_policy;
  currentCache->amount_sets = calculate_number_sets(currentCache);
  currentCache->sets = malloc(sizeof(CacheSet) * currentCache->amount_sets);
  currentCache->hit_miss.read_hit = 0;
  currentCache->hit_miss.read_miss = 0;
  currentCache->hit_miss.write_hit = 0;
  currentCache->hit_miss.write_miss = 0;
  return currentCache;
}

void memory_init(void)
//initializes memory for everything that needs to have allocated memory
{
  srand(time(NULL));
  L1D = (Cache *)malloc(sizeof(Cache));
  L1I = (Cache *)malloc(sizeof(Cache));
  L2 = (Cache *)malloc(sizeof(Cache));

  // initialization for caches without having to hardcode each new cache added:
  cache_initialization(L1I, L1I_size, L1I_associativity,
                       L1I_mapping, L1I_replacement_policy,
                       L1I_line_size, L1I_bus_width, L1I_write_policy);

  cache_initialization(L1D, L1D_size, L1D_associativity,
                       L1D_mapping, L1D_replacement_policy,
                       L1D_line_size, L1D_bus_width, L1D_write_policy);

  cache_initialization(L2, L2_size, L2_associativity,
                       L2_mapping, L2_replacement_policy,
                       L2_line_size, L2_bus_width, L2_write_policy);

  // when added more mappings, change so that the function takes in the mapping as argument
  // and differentiate what is allocated in the function instead of if statements in memory_init.
  if (L1D_mapping == DIRECT_MAPPING)
    allocateDirectMapped(L1D);
  if (L1I_mapping == DIRECT_MAPPING)
    allocateDirectMapped(L1I);
  if (L2_mapping == DIRECT_MAPPING)
    allocateDirectMapped(L2);

  if (L1D_mapping == SET_ASSOCIATIVE_MAPPING)
    allocateSetAssosiativeMapped(L1D);
  if (L1I_mapping == SET_ASSOCIATIVE_MAPPING)
    allocateSetAssosiativeMapped(L1I);
  if (L2_mapping == SET_ASSOCIATIVE_MAPPING)
    allocateSetAssosiativeMapped(L2);

  instr_count = 0;
}

AdressParts GetTagIndexOffset(Cache *currentCache, uint64_t adress)
//function for splitting the adress into tag, index, offset and returning it as a structure containing the tag_index_offset
{
  /*
For finding LSB(least significant bits) use this formula:
masked_bits = ((1<<N)-1)
This will give the N amount of bits as 1's so that together with original adress will output
only the N amount of LSB.

masked_bits & original adress = LSB


For extracting for example bit at position 5 to position 6 from the right lets visualize:
Original bits 0000 1011 1100
bit adress start at position 0 -> 11
If we want to extract bit 01 at position 5 and 6 then we first shift the bits that we dont want out with:
shifted_adress =  adress >> 5  // this gives us 0000 0000 0101
masked_adress = ((1<<2)-1) // this gives us 0000 0000 0011
extracted_2_LSB = masked_adress & shifted_adress // this gives us 0000 0000 0001
which then is the extracted bits at location 5 and 6.

PSEUDO direct mapping associativity:        results:
Offset = log2(block_size)       -              6
index = log2(amount_of_lines)   -              6
tag = adressbits - (offset + index) // aka remaining bits

tag_bit = ((1<<12)-1) & adress      - Gives us the tag bits of the adress
*/
  // calculating offset
  AdressParts tag_index_offset;

  int offsetBits = log2(currentCache->line_size);
  uint64_t offsetMasked = ((1ULL << offsetBits) - 1);
  tag_index_offset.offset = adress & offsetMasked;
  // // printf("offset = %llu\n", offset);

  // Calculating index
  int indexBits = log2(currentCache->amount_sets);
  uint64_t indexMasked = ((1ULL << indexBits) - 1);
  tag_index_offset.indexx = (adress >> offsetBits) & indexMasked;

  // Calculate tag
  tag_index_offset.tag = adress >> (indexBits + offsetBits);
  return tag_index_offset;
}

int CacheLookup(Cache *currentCache, uint64_t adress)
//Function for looking for a adress in the current cache. If the adress is found it returns 1 else it returns 0
{
  AdressParts tag_index_offset = GetTagIndexOffset(currentCache, adress);

  // lookup for directmapping
  if (currentCache->mapping == DIRECT_MAPPING)
  {
    CacheLine *line = &currentCache->sets[tag_index_offset.indexx].lines[0];

    if (line->valid == 1 && line->tag == tag_index_offset.tag)
    {
      return 1;
    }
  }

  if (currentCache->mapping == SET_ASSOCIATIVE_MAPPING)
  {
    CacheSet *set = &currentCache->sets[tag_index_offset.indexx];

    for (int i = 0; i < currentCache->associativity; i++)
    {
      CacheLine *line = &set->lines[i];

      if (line->valid == 1 && line->tag == tag_index_offset.tag)
      {
        return 1;
      }
    }
  }

  return 0; // returns 0 for no found adress with valid bit 1
}
void CacheReplacement(Cache *currentCache, uint64_t address, ReplacementPolicy policy); // Declaring the cachereplacement to be used in cacheinsert

void CacheInsert(Cache *currentCache, uint64_t adress)
//Inserts a adress into the current cache and uses the cachereplacementmethod for evicting the adress in its place if something occupies it.
{
  AdressParts tag_index_off = GetTagIndexOffset(currentCache, adress);

    CacheSet *set = &currentCache->sets[tag_index_off.indexx];

    for (int i = 0; i < currentCache->associativity; i++)
    {
      CacheLine *line = &set->lines[i];
      if (line->valid == 0)
      {
        line->valid = 1;
        line->tag = tag_index_off.tag;
        line->markDirty = NOT_DIRTY;
        return;
      }
    }
    CacheReplacement(currentCache, adress, currentCache->replacement_policy);
  }
// }

void CacheReplacement(Cache *currentCache, uint64_t address, ReplacementPolicy policy)
//Function that evicts a adress based on the replacement policy. Only random replacement policy implemented, but could easily be changed to a different replacement
// by changing the index_to_replace to be equal to the calculation for another replacement policy
{
  // replacement policy for random
  AdressParts tag_index_off = GetTagIndexOffset(currentCache, address);
  int index_to_replace = 0;
  if (policy == RANDOM)
  {
    index_to_replace = rand() % currentCache->associativity;
  }

  CacheSet *set = &currentCache->sets[tag_index_off.indexx];
  CacheLine *replaceLine = &set->lines[index_to_replace];


    /*
    Evicted adress code gotten from chatgpt. Chatlog :
    https://chatgpt.com/share/68f3a9d9-0c8c-8011-8af1-f1bfc5fb46ce
    */
    if (replaceLine->valid && replaceLine->markDirty == DIRTY)
    {
      int offset_bits = log2(currentCache->line_size);
      int index_bits = log2(currentCache->amount_sets);

      uint64_t evicted_address = ((replaceLine->tag << index_bits) | tag_index_off.indexx) << offset_bits;

      if (CacheLookup(L2, evicted_address))
        L2->hit_miss.write_hit++;
      else
      {
        L2->hit_miss.write_miss++;
        CacheInsert(L2, evicted_address);
      }
    }
    replaceLine->valid = 1;
    replaceLine->tag = tag_index_off.tag;
    replaceLine->markDirty = NOT_DIRTY;
}

void MarkDirty(Cache *currentCache, uint64_t address, Dirty dirty)
//Finds the adress that is to be replaced and marks it as dirty. Used for write back write policy
{
  AdressParts tag_index_off = GetTagIndexOffset(currentCache, address);

  if (currentCache->mapping == DIRECT_MAPPING)
  {
    CacheLine *line = &currentCache->sets[tag_index_off.indexx].lines[0];
    line->markDirty = dirty;
  }

  CacheSet *set = &currentCache->sets[tag_index_off.indexx];
  if(currentCache->mapping == SET_ASSOCIATIVE_MAPPING){
    for (size_t i = 0; i < currentCache->associativity; i++)
    {
      CacheLine *line = &set->lines[i];
      if (line->valid && line->tag == tag_index_off.tag)
      {
        line->markDirty = dirty;
        return;
      }
    }
  }
}

void memory_fetch(uint64_t address, data_t *data)
//Fetch instruction call from the cpu
{
  if (CacheLookup(L1I, address))
  {
    L1I->hit_miss.read_hit++;
  }
  else
  {
    L1I->hit_miss.read_miss++;
    if (CacheLookup(L2, address))
    {
      L2->hit_miss.read_hit++;
      CacheInsert(L1I, address); 
    }
    else
    {
      L2->hit_miss.read_miss++;
      CacheInsert(L2, address); 
      CacheInsert(L1I, address); 
    }
  }

  if (data)
    *data = (data_t)0;
  instr_count++;
}

void memory_read(uint64_t address, data_t *data)
//Read instruction from the cpu
{

  if (CacheLookup(L1D, address))
  {
    L1D->hit_miss.read_hit++;
  }
  else
  {
    L1D->hit_miss.read_miss++;
    if (CacheLookup(L2, address))
    {
      L2->hit_miss.read_hit++;
      CacheInsert(L1D, address);
    }
    else
    {

      L2->hit_miss.read_miss++;
      CacheInsert(L2, address);
      CacheInsert(L1D, address);
    }
  }
  if (data)
    *data = (data_t)0;

  instr_count++;
}

void memory_write(uint64_t address, data_t *data)
//Write instruction from the cpu
{

  // --------- WRITE THROUGH POLICY -------- //
  if (L1D->write_policy == WRITE_THROUGH)
  {
    if (CacheLookup(L1D, address))
    {
      L1D->hit_miss.write_hit++;

      if (CacheLookup(L2, address))
      {
        L2->hit_miss.write_hit++;
      }
      else
      {
        L2->hit_miss.write_miss++;
        CacheInsert(L2, address);
      }
    }
    else
    {
      L1D->hit_miss.write_miss++;
      if (CacheLookup(L2, address))
      {
        L2->hit_miss.write_hit++;
        CacheInsert(L1D, address);
      }
      else
      {
        L2->hit_miss.write_miss++;
        CacheInsert(L2, address);
        CacheInsert(L1D, address);
      }
    }
  }
  // --------- WRITE BACK POLICY -------- //

  if (L1D->write_policy == WRITE_BACK)
  {
    if (CacheLookup(L1D, address))
    {
      L1D->hit_miss.write_hit++;
      MarkDirty(L1D, address, DIRTY);
    }
    else
    {
      L1D->hit_miss.write_miss++;
      CacheInsert(L1D,address);
      MarkDirty(L1D,address, DIRTY);
      if (CacheLookup(L2, address))
      {
        L2->hit_miss.write_hit++;
      }
      else
      {
        L2->hit_miss.write_miss++;
        CacheInsert(L2, address);
      }
    }
  }


  instr_count++;
}

void memory_finish(void)
//Prints the total percentages of the hit rates for the different caches hits and misses formatted neatly. 
//Print func has been generated using ai.
{
  printf(" ------- FINISHED SIMULATION --------- \n"); 
  
  // L1D totals
  int L1D_read_total = L1D->hit_miss.read_hit + L1D->hit_miss.read_miss;
  int L1D_write_total = L1D->hit_miss.write_hit + L1D->hit_miss.write_miss;
  int L1D_total = L1D_read_total + L1D_write_total;
  
  // L1I totals
  int L1I_read_total = L1I->hit_miss.read_hit + L1I->hit_miss.read_miss;
  
  // L2 totals
  int L2_read_total = L2->hit_miss.read_hit + L2->hit_miss.read_miss;
  int L2_write_total = L2->hit_miss.write_hit + L2->hit_miss.write_miss;
  int L2_total = L2_read_total + L2_write_total;
  
  // ----------- NEW percentages -----------
  //Logic for calculating the hit rates to percentage 
  //codeblock gotten from chatgpt for easy handling of converting the hitrates to percentages.
  //It checks if the total is greater then 0 and then calculate the percentage with formula 100.0f * cache->hit_miss / L1D_read_total
  //if its not greater then 0 the value is defaulted to 0.
  float L1D_read_hit_rate = (L1D_read_total > 0) ? 100.0f * L1D->hit_miss.read_hit / L1D_read_total : 0.0f;
  float L1D_write_hit_rate = (L1D_write_total > 0) ? 100.0f * L1D->hit_miss.write_hit / L1D_write_total : 0.0f;

  float L1I_read_hit_rate = (L1I_read_total > 0) ? 100.0f * L1I->hit_miss.read_hit / L1I_read_total : 0.0f;

  float L2_read_hit_rate = (L2_read_total > 0) ? 100.0f * L2->hit_miss.read_hit / L2_read_total : 0.0f;
  float L2_write_hit_rate = (L2_write_total > 0) ? 100.0f * L2->hit_miss.write_hit / L2_write_total : 0.0f;

  // Original total hit-rates from your version
  float L1D_hit_rate = (L1D_total > 0) ? 100.0f * (L1D->hit_miss.read_hit + L1D->hit_miss.write_hit) / L1D_total : 0.0f;

  float L1I_hit_rate = (L1I_read_total > 0) ? 100.0f * L1I->hit_miss.read_hit / L1I_read_total : 0.0f;

  float L2_hit_rate = (L2_total > 0) ? 100.0f * (L2->hit_miss.read_hit + L2->hit_miss.write_hit) / L2_total : 0.0f;

  // ----------- PRINT EXACT SAME FORMAT + EXTRA INFO -----------

  printf("-- L1I -- Read_Hits: %d  Read_Miss: %d  [Hit Rate: %.2f%%]  (Read Hit%%: %.2f%%)\n",
         L1I->hit_miss.read_hit, L1I->hit_miss.read_miss,
         L1I_hit_rate,
         L1I_read_hit_rate);

  printf("-- L1D -- Read_Hits: %d  Read_Miss: %d  Write_Hits: %d  Write_Miss: %d  [Hit Rate: %.2f%%]\n",
         L1D->hit_miss.read_hit, L1D->hit_miss.read_miss,
         L1D->hit_miss.write_hit, L1D->hit_miss.write_miss,
         L1D_hit_rate);

  printf("          (Read Hit%%: %.2f%%   Write Hit%%: %.2f%%)\n",
         L1D_read_hit_rate, L1D_write_hit_rate);

  printf("-- L2  -- Read_Hits: %d  Read_Miss: %d  Write_Hits: %d  Write_Miss: %d  [Hit Rate: %.2f%%]\n",
         L2->hit_miss.read_hit, L2->hit_miss.read_miss,
         L2->hit_miss.write_hit, L2->hit_miss.write_miss,
         L2_hit_rate);

  printf("          (Read Hit%%: %.2f%%   Write Hit%%: %.2f%%)\n",
         L2_read_hit_rate, L2_write_hit_rate);

  printf("Executed %lu instructions.\n\n", instr_count);
}
