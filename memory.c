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

typedef struct {
  int data;
  int size;
}Cache;

Cache *L1D;
Cache *L1I;
Cache *L2;


void memory_init(void)
{
  L1D = (Cache*)malloc(sizeof(Cache));
  L1I = (Cache*)malloc(sizeof(Cache));
  L2 = (Cache*)malloc(sizeof(Cache));

  L1D->data = 0;
  L1D->size = 0;

  L1I->data = 0;
  L1I->size = 0;

  L2->data = 0;
  L2->size = 0;


  printf("initializing memory\n");
  
  /* Initialize memory subsystem here. */

  instr_count = 0;
}

void memory_fetch(uint64_t address, data_t *data)
{
  printf("memory: fetch 0x%" PRIx64 "\n", address);
  if (data)
    *data = (data_t)0;

  instr_count++;
}

void memory_read(uint64_t address, data_t *data)
{
  printf("memory: read 0x%" PRIx64 "\n", address);
  if (data)
    *data = (data_t)0;

  instr_count++;
}

void memory_write(uint64_t address, data_t *data)
{
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