#ifndef INCLUDE_MM_H_
#define INCLUDE_MM_H_

#include "type.h"
#include "queue.h"

#define TLB_NUM 32

void do_TLB_Helper();
void do_page_fault();


#define PGSIZE           4096    //page: 4KB
#define PTE_NUM          1024*128 //USR STACK: 0 - 0x80000000 

// 31                               6 5  3  2   1   0 
// +-----------------+---------------+----+---+---+---+
// |              PFN                |  C | D | V | G |
// +-----------------+---------------+----+---+---+---+

#define PTE_C            2 //Cache
#define PTE_D            1 //Dirty
#define PTE_V            1 //Valid
#define PTE_G            1 //Global


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

pgframe_t pf_group[PAGE_FRAMES_NUM];
queue_t emptylist;
queue_t fulllist;
void free_pgframe(uint32_t, uint32_t);

#endif
