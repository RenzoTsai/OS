#include "irq.h"
#include "time.h"
#include "sched.h"
#include "string.h"
#include "screen.h"

static void irq_timer()
{
    // TODO clock interrupt handler.
    // scheduler, time counter in here to do, emmmmmm maybe.
    screen_reflush();
    time_elapsed += TIMER_INTERVAL;
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
        irq_timer();
    else
        other_exception_handler();
}

