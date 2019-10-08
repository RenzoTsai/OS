/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this 
 * software and associated documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit 
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE. 
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include "irq.h"
#include "test.h"
#include "stdio.h"
#include "sched.h"
#include "screen.h"
#include "common.h"
#include "syscall.h"

queue_t ready_queue;
queue_t block_queue;
uint32_t initial_cp0_status;
uint32_t queue_id;
uint32_t exception_handler[32];

static void init_pcb()
{
	int i,j;
	pcb[0].pid=process_id++;
	pcb[0].status=TASK_RUNNING;
	int stack_top=STACK_MAX;
	queue_id=1;
	
	queue_init(&ready_queue);
	queue_init(&block_queue);
	for(i=0;i<num_sched1_tasks;i++,queue_id++){
		for(j=0;j<32;j++){
			pcb[queue_id].kernel_context.regs[j]=0;
			pcb[queue_id].user_context.regs[j]=0;
		}
		pcb[queue_id].pid=process_id++;
		pcb[queue_id].type=sched1_tasks[i]->type;
		pcb[queue_id].status=TASK_READY;

		pcb[queue_id].kernel_stack_top=stack_top;
		pcb[queue_id].kernel_context.regs[29]=stack_top;
		stack_top-=STACK_SIZE;

		pcb[queue_id].kernel_context.regs[31]= sched1_tasks[i]->entry_point;
		pcb[queue_id].kernel_context.cp0_epc = sched1_tasks[i]->entry_point;
		pcb[queue_id].kernel_context.cp0_status=0x30008001;


		pcb[queue_id].user_stack_top=stack_top;
		pcb[queue_id].user_context.regs[29]=stack_top;
		stack_top-=STACK_SIZE;

		pcb[queue_id].user_context.regs[31]=sched1_tasks[i]->entry_point;
		pcb[queue_id].user_context.cp0_epc=sched1_tasks[i]->entry_point;
		pcb[queue_id].user_context.cp0_status=0x30008001;
		queue_push(&ready_queue,(void *)&pcb[queue_id]);
	}

	for(i=0;i<num_sched2_tasks;i++,queue_id++){
		for(j=0;j<32;j++){
			pcb[queue_id].kernel_context.regs[j]=0;
			pcb[queue_id].user_context.regs[j]=0;
		}
		pcb[queue_id].pid=process_id++;
		pcb[queue_id].type=sched2_tasks[i]->type;
		pcb[queue_id].status=TASK_READY;

		pcb[queue_id].kernel_stack_top=stack_top;
		pcb[queue_id].kernel_context.regs[29]=stack_top;

		stack_top-=STACK_SIZE;

		pcb[queue_id].kernel_context.regs[31]= sched2_tasks[i]->entry_point;
		pcb[queue_id].kernel_context.cp0_epc = sched2_tasks[i]->entry_point;
		pcb[queue_id].kernel_context.cp0_status=0x30008001;

		pcb[queue_id].user_stack_top=stack_top;
		pcb[queue_id].user_context.regs[29]=stack_top;
		stack_top-=STACK_SIZE;

		pcb[queue_id].user_context.regs[31]=sched2_tasks[i]->entry_point;
		pcb[queue_id].user_context.cp0_epc=sched2_tasks[i]->entry_point;
		pcb[queue_id].user_context.cp0_status=0x30008001;
		queue_push(&ready_queue,(void *)&pcb[queue_id]);
	}


	 current_running=&pcb[0];
}

static void init_exception_handler()
{
	int i;
	//copy handle function to exception_handler
	exception_handler[0]=&handle_int;
	for(i=1;i<32;i++)
		exception_handler[i]=&handle_other;
	exception_handler[8]=&handle_syscall;
}

static void init_exception()
{
	init_exception_handler();
	// 1. Get CP0_STATUS
	initial_cp0_status = GET_CP0_STATUS();

	// 2. Disable all interrupt
	initial_cp0_status = 0x30008001;
	initial_cp0_status &=0xfffffffe;
	SET_CP0_STATUS(initial_cp0_status);
	initial_cp0_status|= 0x1;
	
	// 3. Copy the level 2 exception handling code to 0x80000180
	//bzero((void *)BEV0_EBASE, BEV0_OFFSET);
	memcpy((void *)(BEV0_EBASE+BEV0_OFFSET), &exception_handler_entry, exception_handler_end-exception_handler_begin);
	//bzero((void *)BEV1_EBASE, BEV1_OFFSET);
	memcpy((void *)(BEV1_EBASE+BEV1_OFFSET), &exception_handler_entry, exception_handler_end-exception_handler_begin);

	// 4. reset CP0_COMPARE & CP0_COUNT register
	SET_CP0_COMPARE(TIMER_INTERVAL);
	SET_CP0_COUNT(0);
	
}

static void init_syscall(void)
{

	// init system call table.
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
void __attribute__((section(".entry_function"))) _start(void)
{
	// Close the cache, no longer refresh the cache 
	// when making the exception vector entry copy

	asm_start();

	// init interrupt (^_^)
	init_exception();
	printk("> [INIT] Interrupt processing initialization succeeded.\n");

	// init system call table (0_0)
	init_syscall();
	printk("> [INIT] System call initialized successfully.\n");

	// init Process Control Block (-_-!)
	init_pcb();
	printk("> [INIT] PCB initialization succeeded.\n");

	// init screen (QAQ)
	init_screen();
	printk("> [INIT] SCREEN initialization succeeded.\n");

	// TODO Enable interrupt
	SET_CP0_STATUS(initial_cp0_status);
	
	while (1)
	{
		// (QAQQQQQQQQQQQ)
		// If you do non-preemptive scheduling, you need to use it to surrender control
		 // do_scheduler();
	};
	return;
}
