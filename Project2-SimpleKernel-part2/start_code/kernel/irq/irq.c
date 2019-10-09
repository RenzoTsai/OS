#include "irq.h"
#include "time.h"
#include "sched.h"
#include "string.h"
#include "screen.h"

static void irq_timer()
{
	screen_reflush();
	time_elapsed += TIMER_INTERVAL;
	current_running->cursor_x = screen_cursor_x;
    current_running->cursor_y = screen_cursor_y;
    do_scheduler();
    screen_cursor_x = current_running->cursor_x;
    screen_cursor_y = current_running->cursor_y;
    // TODO clock interrupt handler.
    // scheduler, time counter in here to do, emmmmmm maybe.
}

void other_exception_handler()
{
	time_elapsed += TIMER_INTERVAL;

    // TODO other exception handler
}

void interrupt_helper(uint32_t status, uint32_t cause)
{
	int interrupt;
	interrupt = status & cause & 0x0000ff00;
    if(interrupt & 0x00008000)
        irq_timer();
    else
        other_exception_handler();

    // TODO interrupt handler.
    // Leve3 exception Handler.
    // read CP0 register to analyze the type of interrupt.
}

