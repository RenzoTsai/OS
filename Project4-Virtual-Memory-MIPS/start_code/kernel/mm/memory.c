#include "mm.h"
#include "sched.h"

//TODO:Finish memory management functions here refer to mm.h and add any functions you need.
pgframe_t *palloc()
{
    pgframe_t *item = (pgframe_t *)emptylist.head;
    
    if(emptylist.head == emptylist.tail)
        emptylist.head = emptylist.tail = NULL;
    else
    {
        emptylist.head = (void *)item->next;
        item->next->prev = NULL;
    }

    if(fulllist.head == NULL)
    {
        fulllist.head = (void *)item;
        fulllist.tail = (void *)item;
        item->next = item->prev = NULL;
    }
    else
    {
        ((pgframe_t *)fulllist.tail)->next = item;
        item->next = NULL;
        item->prev = (pgframe_t *)fulllist.tail;
        fulllist.tail = (void *)item;
    }

    return item;
}

void pgframe_init(pgframe_t *item)
{
    if(item->prev && item->next)
    {
        void *_item = (void *)item;
        if (_item == fulllist.head && _item == fulllist.tail)
        {
            fulllist.head = NULL;
            fulllist.tail = NULL;
        }
        else if (_item == fulllist.head)
        {
            fulllist.head = (void *)item->next;
            ((pgframe_t *)(fulllist.head))->prev = NULL;
        }
        else if (_item == fulllist.tail)
        {
            fulllist.tail = (void *)item->prev;
            ((pgframe_t *)(fulllist.tail))->next = NULL;
        }
        else
        {
            (item->prev)->next = item->next;
            (item->next)->prev = item->prev;
        }
    }
    if(emptylist.head == NULL)
    {
        emptylist.head = (void *)item;
        emptylist.tail = (void *)item;
        item->next = item->prev = NULL;
    }
    else
    {
        ((pgframe_t *)emptylist.tail)->next = item;
        item->next = NULL;
        item->prev = (pgframe_t *)emptylist.tail;
        emptylist.tail = (void *)item;
    }
    // queue_remove(&fulllist,item);
     //queue_push(&emptylist,item);
}

void free_pgframe(uint32_t p_start, uint32_t p_end)
{
    uint32_t p, i;
    for(i = 0,p = p_start; p + PGSIZE < p_end; p += PGSIZE, i++)
    {
        pf_group[i].paddr = p;
        pf_group[i].vpte = NULL;
        pf_group[i].prev = pf_group[i].next  = NULL;
        pgframe_init(&pf_group[i]);
    }
}

void do_TLB_Helper()
{
    uint32_t entryhi  = get_cp0_entryhi();
    uint32_t context  = get_cp0_context();
    //uint32_t badvaddr = get_cp0_badvaddr();
    //uint32_t vpn2=(badvaddr>>1)<<13;
    //do_print("badvaddr:%x",badvaddr);
    context = context << 9; //得到BadVPN2(低12位置0)
    set_cp0_entryhi(context | entryhi & 0xff);  
    asm volatile("tlbp");  //invalid 在填充TLB项之前，使用TLBP指令来找出匹配但是无效的那一项的index值
    uint32_t index = get_cp0_index();
    uint32_t entrylo0, entrylo1;
    //do_print("current_running:%d entryhi:%d V:%d G:%d \n",(current_running->pid& 0xff),entryhi & 0xff,(current_running->pagetable[context >> 12] & (PTE_V<<1)),(current_running->pagetable[context >> 12] & (PTE_G)));
 
    if(index & 0x80000000) {  //TLBP没有成功时P置 1
        //TLB refill
        // if(((current_running->pid& 0xff)!=entryhi & 0xff) && (current_running->pagetable[context >> 12] & (PTE_V<<1)) && !(current_running->pagetable[context >> 12] & (PTE_G))){
        //     do_print("ASID ERROR! current_running:%d entryhi:%d V:%d G:%d \n",(current_running->pid& 0xff),entryhi & 0xff,(current_running->pagetable[context >> 12] & (PTE_V<<1)),(current_running->pagetable[context >> 12] & (PTE_G)));
        //     do_exit();
        // }
        entryhi = (context &0xffffff00) | (current_running->pid& 0xff);
        set_cp0_entryhi(entryhi);
          // entrylo0 = (current_running->pagetable[context >> 12]| PTE_C<<3 | PTE_D<<2 | PTE_V<<1 )&(0xfffffffe) ;
          // entrylo1 = (current_running->pagetable[context >> 12 | 1] | PTE_C<<3 | PTE_D<<2 | PTE_V<<1)&(0xfffffffe) ;
        entrylo0 = (current_running->pagetable[context >> 12]| (PTE_C<<3)  ) &(0xfffffffe);
        entrylo1 = (current_running->pagetable[context >> 12 | 1] | (PTE_C<<3) ) &(0xfffffffe);
        set_cp0_entrylo0(entrylo0);
        set_cp0_entrylo1(entrylo1);
        asm volatile("tlbwr"); //随机填充TLB
    }
    else{
        //TLB invalid
        if(!(current_running->pagetable[context >> 12] & (PTE_V<<1))){  //成功找到TLB索引值但无效
            if(emptylist.head != NULL){
                pgframe_t *frame;
                frame = palloc(); //偶：分配一个可利用的物理页框
                //current_running->pagetable[context >> 12] = ((frame->paddr & 0xfffff000)>>6 | PTE_C<<3 | PTE_D<<2 | PTE_V<<1)&(0xfffffffe);
                current_running->pagetable[context >> 12] = ((frame->paddr & 0xfffff000)>>6 | (PTE_C<<3) | (PTE_D<<2) | (PTE_V<<1));
                frame->vpte = &current_running->pagetable[context >> 12];
                frame->vaddr = context | current_running->pid<<2;
                frame->is_valid=1;
                frame = palloc(); //奇：分配一个可利用的物理页框
                //current_running->pagetable[context >> 12 | 1] = ((frame->paddr & 0xfffff000)>>6 | PTE_C<<3 | PTE_D<<2 | PTE_V<<1)&(0xfffffffe);
                current_running->pagetable[context >> 12 | 1] = ((frame->paddr & 0xfffff000)>>6 | (PTE_C<<3) | (PTE_D<<2) | (PTE_V<<1));
                frame->vpte = &current_running->pagetable[context >> 12 | 1];
                frame->vaddr = (context | 1 << 12) | current_running->pid<<2;
                frame->is_valid=1;
            }
        }
         // entrylo0 = current_running->pagetable[context >> 12] &(0xfffffffe);
         // entrylo1 = current_running->pagetable[context >> 12 | 1]&(0xfffffffe);
        entryhi = (context &0xffffff00) | (current_running->pid& 0xff);
        entrylo0 = current_running->pagetable[context >> 12] ;
        entrylo1 = current_running->pagetable[context >> 12 | 1];
        //set_cp0_entryhi(entryhi);
        set_cp0_entrylo0(entrylo0);
        set_cp0_entrylo1(entrylo1);
        asm volatile("tlbwi"); //根据INDEX填充TLB
    }
}







