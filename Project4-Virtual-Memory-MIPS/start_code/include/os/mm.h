#ifndef INCLUDE_MM_H_
#define INCLUDE_MM_H_

#include "type.h"

#define TLB_ENTRY_NUMBER 32

void do_TLB_Helper();
void do_page_fault();


#define PGSIZE           4096    //page: 4KB
#define PTENTRY_NUM      1024*1024 //32M / 4K = 8K

#define PTXSHIFT         12      //offset of PTX in a linear address

#define GETPTX(va)          (((uint32_t)va >> PTXSHIFT) & 0xfffff)

// 31                             13  12 10 9   8   7 
// +-----------------+---------------+----+---+---+---+
// |              PFN                |  C | D | V | G |
// +-----------------+---------------+----+---+---+---+

#define PTE_C            0x400 //Cache
#define PTE_D            0x100 //Dirty
#define PTE_V            0x080 //Valid
#define PTE_G            0x040 //Global


typedef uint32_t pte_t;

//free physical space
#define PAGE_FRAMES_NUM         1024*8 //page frame number
typedef struct pgframe {
    uint32_t paddr; //physical addr
    uint32_t vaddr; //virtual addr
    pte_t * vpte;
    uint8_t is_valid;
    struct pgframe *prev;
    struct pgframe *next;
} pgframe_t;

pgframe_t pmem[PAGE_FRAMES_NUM];

// queue_t emptylist;
// queue_t fulllist;

void free_pgframe(uint32_t, uint32_t);

#endif
