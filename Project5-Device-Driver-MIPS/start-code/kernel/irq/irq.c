#include "irq.h"
#include "time.h"
#include "sched.h"
#include "string.h"
#include "screen.h"
#include "mac.h"

static void irq_timer()
{
    // TODO clock interrupt handler.
    // scheduler, time counter in here to do, emmmmmm maybe.
    screen_reflush();
    time_elapsed += TIMER_INTERVAL;
    //ONLY USE IN TASK2 WAKE UP "recv_block_queue" IN TIME IRQ:
    // mac_recv_handle();
    // if(!queue_is_empty(&recv_block_queue) && !((*(uint32_t *)recv_flag[ch_flag]) & 0x80000000))
    //     do_unblock_one(&recv_block_queue);
    do_scheduler();
}

void other_exception_handler()
{
    // TODO other exception handler
	time_elapsed += TIMER_INTERVAL;
}

void interrupt_helper(uint32_t status, uint32_t cause)
{
    // TODO interrupt handler.
    // Leve3 exception Handler.
    // read CP0 register to analyze the type of interrupt.
    int interrupt;
    interrupt = status & cause & 0x0000ff00;
    if(interrupt & 0x00008000)
        irq_timer(interrupt & 0x00008000);
    else if((interrupt & 0x00008000) &&(reg_read_32(0xbfd01058) & 0x8))
        mac_irq_handle();
    else
        other_exception_handler();
}

