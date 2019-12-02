#include "sched.h"
#include "stdio.h"
#include "syscall.h"
#include "time.h"
#include "screen.h"
#include "test4.h"

#define RW_TIMES 3

int rand()
{	
	int current_time = get_timer();
	return current_time % 100000;
}

static void disable_interrupt()
{
    uint32_t cp0_status = GET_CP0_STATUS();
    cp0_status &= 0xfffffffe;
    SET_CP0_STATUS(cp0_status);
}

static void enable_interrupt()
{
    uint32_t cp0_status = GET_CP0_STATUS();
    cp0_status |= 0x01;
    SET_CP0_STATUS(cp0_status);
}

// static char read_uart_ch(void)
// {
//     char ch = 0;
//     unsigned char *read_port = (unsigned char *)(0xbfe48000 + 0x00);
//     unsigned char *stat_port = (unsigned char *)(0xbfe48000 + 0x05);

//     while ((*stat_port & 0x01))
//     {
//         ch = *read_port;
//     }
//     return ch;
// }

//int memory[RW_TIMES];
void rw_task1(void)
{
	int mem1, mem2 = 0;
	int curs = 0;
	int memory[RW_TIMES];
	int i = 0;
	uint32_t rw_task1_ipt[6]={0x20001000,0x20002000,0x20003000,0x20001000,0x20002000,0x20003000};
	for(i = 0; i < RW_TIMES; i++)
	{
		mem1 = rw_task1_input[i];
		sys_move_cursor(1, curs+i);
		memory[i] = mem2 = rand();
		// disable_interrupt();
		*(int *)mem1 = mem2;
		// enable_interrupt();
		printf("Write: 0x%x, %d", mem1, mem2);
	}
	curs = RW_TIMES;
	for(i = 0; i < RW_TIMES; i++)
	{
		mem1 = rw_task1_input[RW_TIMES+i];
		sys_move_cursor(1, curs+i);
		// disable_interrupt();
		memory[i+RW_TIMES] = *(int *)mem1;
		// enable_interrupt();
		if(memory[i+RW_TIMES] == memory[i])
			printf("Read succeed: %d", memory[i+RW_TIMES]);
		else
			printf("Read error: %d", memory[i+RW_TIMES]);
	}
	while(1);
	//Input address from argv.
}

void rw_task2(void)
{
	int mem1, mem2 = 0;
	int curs = 0;
	int memory[RW_TIMES];
	int i = 0;
	// for(i = 0; i < RW_TIMES; i++)
	// {
	// 	mem1 = rw_task1_ipt[i];
	// 	sys_move_cursor(1, curs+i);
	// 	memory[i] = mem2 = rand();
	// 	*(int *)mem1 = mem2;
	// 	printf("Write: 0x%x, %d", mem1, mem2);
	// }
	curs = RW_TIMES;
	for(i = 0; i < RW_TIMES; i++)
	{
		mem1 = rw_task1_input[RW_TIMES+i];
		sys_move_cursor(1, curs+i);
		disable_interrupt();
		memory[i+RW_TIMES] = *(int *)mem1;
		enable_interrupt();
		printf("Addr:%x Read: %d",mem1, memory[i+RW_TIMES]);
		
	}
	while(1);
	//Input address from argv.
}
