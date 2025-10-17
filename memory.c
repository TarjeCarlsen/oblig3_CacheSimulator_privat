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

typedef struct{
  uint64_t tag;
  uint64_t indexx; // Intentionally mispelled, index is a legacy function
  uint64_t offset;
}AdressParts;

typedef struct { // size, associativity, line size, write back policy should be easily changeable.
  int size; // number of sets = cache size / (line_size * associativity) //  depends on the associativity
  int associativity;
  int mapping;
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
#define L1D_size 3000 //32kb
#define L1D_associativity 1 // set to 1 for testing direct mapping
#define L1D_mapping DIRECT_MAPPING // set to 1 for testing direct mapping
#define L1D_replacement_policy RANDOM
#define L1D_line_size 64
#define L1D_bus_width 32
#define L1D_write_policy WRITE_BACK

#define L1I_size 32000 // 32kb
#define L1I_associativity 1
#define L1I_mapping DIRECT_MAPPING // set to 1 for testing direct mapping
#define L1I_replacement_policy RANDOM
#define L1I_line_size 64
#define L1I_bus_width 32
#define L1I_write_policy WRITE_BACK

#define L2_size 512000 //512kb
#define L2_associativity 1
#define L2_mapping DIRECT_MAPPING // set to 1 for testing direct mapping
#define L2_write_policy WRITE_BACK
#define L2_line_size 64
#define L2_replacement_policy RANDOM
#define L2_bus_width 32

void print_bits(uint64_t value, int bits)
{
    for (int i = bits - 1; i >= 0; i--) {
        printf("%d", (int)((value >> i) & 1ULL));  // cast fixes warning
        if (i % 4 == 0) printf(" ");               // optional spacing
    }
    printf("\n");
}

int calculate_number_sets(Cache *currentCache){
  int result = 0;
  if(currentCache->mapping == DIRECT_MAPPING){
    result = currentCache->size / (currentCache->line_size * currentCache->associativity);
    return result;
  }
  return 0;
}

void allocateDirectMapped(Cache *currentCache){
  for (size_t i = 0; i < currentCache->amount_sets; i++)
  {
    currentCache->sets[i].lines = malloc(sizeof(CacheLine) * currentCache->associativity);
    for (size_t j = 0; j < currentCache->associativity; j++)
    {
      currentCache->sets[i].lines[j].valid = 0;
      currentCache->sets[i].lines[j].tag = 0;
    }
    
  }
  
}

void memory_init(void)
{
  L1D = (Cache*)malloc(sizeof(Cache));
  L1I = (Cache*)malloc(sizeof(Cache));
  L2 = (Cache*)malloc(sizeof(Cache));

  L1D->size = L1D_size;
  L1D->associativity = L1D_associativity;
  L1D->mapping = L1D_mapping;
  L1D->inclusive;
  L1D->replacement_policy = L1D_replacement_policy;
  L1D->line_size = L1D_line_size;
  L1D->bus_width = L1I_bus_width;
  L1D->write_policy = L1D_write_policy;
  L1D->miss = 0;
  L1D->hit = 0;
  L1D->amount_sets = calculate_number_sets(L1D);
  L1D->sets = malloc(sizeof(CacheSet) * L1D->amount_sets);
  
  L1I->size = L1I_size;
  L1I->associativity = L1I_associativity;
  L1I->mapping = L1I_mapping;
  L1I->inclusive;
  L1I->replacement_policy = L1I_replacement_policy;
  L1I->line_size = L1I_line_size;
  L1I->bus_width = L1I_bus_width;
  L1I->write_policy = L1I_write_policy;
  L1I->miss = 0;
  L1I->hit = 0;
  L1I->amount_sets = calculate_number_sets(L1I);
  L1I->sets = malloc(sizeof(CacheSet) * L1I->amount_sets);
  
  L2->size = L2_size;
  L2->associativity = L2_associativity;
  L2->mapping = L2_mapping;
  L2->inclusive;
  L2->replacement_policy = L2_replacement_policy;
  L2->line_size = L2_line_size;
  L2->bus_width = L2_bus_width;
  L2->write_policy = L2_write_policy;
  L2->miss = 0;
  L2->hit = 0;
  L2->amount_sets = calculate_number_sets(L2);
  L2->sets = malloc(sizeof(CacheSet) * L2->amount_sets);
  
  if(L1D_mapping == DIRECT_MAPPING)allocateDirectMapped(L1D);
  if(L1I_mapping == DIRECT_MAPPING)allocateDirectMapped(L1I);
  if(L2_mapping == DIRECT_MAPPING)allocateDirectMapped(L2);

  instr_count = 0;
}

AdressParts GetTagIndexOffset(Cache *currentCache, uint64_t adress){
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

  PSEUDO direct mapping associativity:        results:
  Offset = log2(block_size)       -              6
  index = log2(amount_of_lines)   -              6
  tag = adressbits - (offset + index) // aka remaining bits        

  tag_bit = ((1<<12)-1) & adress      - Gives us the tag bits of the adress
  */
  // calculating offset
    AdressParts tag_index_offset;

    int offsetBits = log2(currentCache->line_size);
    uint64_t offsetMasked = ((1ULL<<offsetBits)-1);
    tag_index_offset.offset = adress & offsetMasked;
    // printf("offset = %llu\n", offset);

  // Calculating index
    int indexBits = log2(currentCache->amount_sets);
    uint64_t indexMasked = ((1ULL<<indexBits) -1 );
    tag_index_offset.indexx = (adress >> offsetBits) & indexMasked;

    //Calculate tag
    tag_index_offset.tag = adress>>(indexBits + offsetBits);
  return tag_index_offset;
}

int CacheLookup(Cache *currentCache, uint64_t adress){
  AdressParts tag_index_offset = GetTagIndexOffset(currentCache,adress);

    //lookup for directmapping
    if(currentCache->mapping == DIRECT_MAPPING){
      CacheLine *line = &currentCache->sets[tag_index_offset.indexx].lines[0];

      if(line->valid == 1 && line->tag == tag_index_offset.tag){
        return 1;
      }else{
        return 0;
      }
    }
  return 0; // returns 0 for no found adress with valid bit 1
}

void CacheInsert(Cache *currentCache, uint64_t adress){
  AdressParts tag_index_off = GetTagIndexOffset(currentCache,adress);

  //insert for direct mapping
  if(currentCache->mapping == DIRECT_MAPPING){
    CacheLine *line = &currentCache->sets[tag_index_off.indexx].lines[0];

    line->valid = 1;
    line->tag = tag_index_off.tag;
  }
}

void memory_fetch(uint64_t address, data_t *data)
{
  /* PSEUDO
  CHECK IF L1I HITS OR MISSES
  if(cache_lookup(L1I, adress))
  L1I->hits++;
  else
  L1I->miss++;
  
  CHECK IF L2 HITS OR MISSES
  if(cache_lookup(L2,adress))
  L2->hits++;
  cache->insert(L1I, adress)
  else
  L2->misses++;
  cache_insert(L2, adress)
  cachce_insert(L1I, adress)
  */
  if(CacheLookup(L1I,address)){
    L1I->hit++;
  }else{
    L1I->miss++;
    if(CacheLookup(L2, address)){
      L2->hit++;
      CacheInsert(L1I, address); // must insert to L1I on hit
    }else{
      L2->miss++;
      CacheInsert(L2,address); // must insert to L2 and L1I on miss
      CacheInsert(L1I, address); // must insert to L2 and L1I on miss
    }
  }
  printf("L1I hits = %d miss = %d\n", L1I->hit, L1I->miss);
  printf("L2 hits = %d miss = %d\n", L2->hit, L2->miss);
  
  printf("data = %p \n", (void*)data);
  printf("memory: fetch 0x%" PRIx64 "\n", address);
  if (data)
  *data = (data_t)0;
  
  //  CacheLookup(L1D, address);
  instr_count++;
}

void memory_read(uint64_t address, data_t *data)
{
  /* PSEUDO
  CHECK IF L1I HITS OR MISSES
  if(cache_lookup(L1D, adress))
  L1I->hits++;
  else
  L1I->miss++;
  
  CHECK IF L2 HITS OR MISSES
  if(cache_lookup(L2,adress))
  L2->hits++;
  cache_insert(L1D, adress)
  else
  L2->misses++;
  cache_insert(L2, adress)
  cachce_insert(L1D, adress)
  */
 
 if(CacheLookup(L1D,address)){
   L1D->hit++;
  }else{
    L1D->miss++;
    if(CacheLookup(L2,address)){
      L2->hit++;
      CacheInsert(L1D,address);
    }else{
      L2->miss++;
      CacheInsert(L2,address);
      CacheInsert(L1D,address);
    }
  }
  printf("L1D hits = %d miss = %d\n", L1D->hit, L1D->miss);
  printf("L2 hits = %d miss = %d\n", L2->hit, L2->miss);
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