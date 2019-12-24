#include "irq.h"
#include "time.h"
#include "sched.h"
#include "string.h"
#include "screen.h"
#include "mac.h"
#include "stdio.h"

static void irq_timer()
{
    // TODO clock interrupt handler.
    // scheduler, time counter in here to do, emmmmmm maybe.
    screen_reflush();
    current_running->cursor_x = screen_cursor_x;
    current_running->cursor_y = screen_cursor_y;
    time_elapsed += TIMER_INTERVAL;
    //ONLY USE IN TASK2 WAKE UP "recv_block_queue" IN TIME IRQ:
    // if(!queue_is_empty(&recv_block_queue)){
    //     check_recv(&test_mac2);
    // }

    do_scheduler();
    screen_cursor_x = current_running->cursor_x;
    screen_cursor_y = current_running->cursor_y;
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
    else if((interrupt & 0x0000800) &&(reg_read_32(0xbfd01058) & 0x8))
        mac_irq_handle();
    else
        other_exception_handler();
}

