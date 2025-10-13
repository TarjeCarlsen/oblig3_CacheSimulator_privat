#ifndef BYUTR_H
#define BYUTR_H

#include <stdint.h>
typedef struct BYUADDRESSTRACE
{
  uint64_t addr;
  uint8_t reqtype;
  uint8_t size;
  uint8_t attr;
  uint8_t proc;
  uint32_t time;
} p2AddrTr;

/* reqtype values */
#define FETCH 0x00      // instruction fetch
#define MEMREAD 0x01    // memory read
#define MEMREADINV 0x02 // memory read and invalidate
#define MEMWRITE 0x03   // memory write

#define IOREAD 0x10  // IO read
#define IOWRITE 0x11 // IO Write

#define DEFERREPLY 0x20   // deferred reply
#define INTA 0x21         // interrupt acknowledge
#define CNTRLAGNTRES 0x22 // central agent response
#define BRTRACEREC 0x23   // branch trace record

#define SHUTDOWN 0x31   // shutdown
#define FLUSH 0x32      // flush
#define HALT 0x33       // halt
#define SYNC 0x34       // sync
#define FLUSHACK 0x35   // acknowledge flush
#define STOPCLKACK 0x36 // acknowledge stop clock
#define SMIACK 0x37     // acknowledge SMI mode

#ifdef BIG_ENDIAN
/* If you are using this program on a big-endian machine
  (something other than an Intel PC or equivalent) you will need to
  use this function on tr.addr and tr.time.  Just replace references to
  tr.addr   with   swap_endian(tr.addr)
  Exact match with glibc */

/**/

uint64_t swap_endian(uint64_t num)
{
  return ((((num) & 0xff00000000000000ull) >> 56) |
          (((num) & 0x00ff000000000000ull) >> 40) |
          (((num) & 0x0000ff0000000000ull) >> 24) |
          (((num) & 0x000000ff00000000ull) >> 8) |
          (((num) & 0x00000000ff000000ull) << 8) |
          (((num) & 0x0000000000ff0000ull) << 24) |
          (((num) & 0x000000000000ff00ull) << 40) |
          (((num) & 0x00000000000000ffull) << 56));
}

/* this might be useful */

int is_big_endian(void)
{
  uint32_t *a;
  uint8_t p[4];

  a = (uint32_t *)p;
  *a = 0x12345678;
  if (*p == 0x12)
  {
    fprintf(stderr, "Big Endian System\n");
    return 1;
  }
  else
  {
    fprintf(stderr, "Little Endian System\n");
    return 0;
  }
}
#endif // big_endian

#endif // byutr.h