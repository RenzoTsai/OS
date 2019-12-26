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
#include "time.h"
#include "sync.h"
#include "mac.h"
#include "fs.h"

queue_t ready_queue;
queue_t exit_queue;
queue_t sleep_queue;

uint32_t initial_cp0_status;
uint32_t queue_id;
uint32_t exception_handler[32];
uint32_t lock_id;
uint32_t reuse_stack[40];
int reuse_stack_top = -1;
int stack_top = STACK_MAX;


// static void init_page_table()
// {
// 	// int i;
// 	// for(i=0;i<PTE_NUM;i++){
// 	// 	pgtable[i]=PTE_C<<3|PTE_D<<2|PTE_V<<1|PTE_G;
// 	// }
// 	// uint32_t vpn2=0x10000;
// 	// uint32_t epfn=3;
// 	// for(i=0;i<TLB_NUM;i++,vpn2++,epfn++){
// 	// 	pgtable[vpn2<<13]=epfn<<6|PTE_C<<3|PTE_D<<2|PTE_V<<1|PTE_G;
// 	// 	pgtable[(vpn2<<13)|(1<<12)]=epfn<<6|PTE_C<<3|PTE_D<<2|PTE_V<<1|PTE_G;		
// 	// }
// }

// static void init_TLB()
// {
// 	int i;
// 	uint32_t vpn2=0x10000;
// 	uint32_t asid=0,pagemask=0;
// 	uint32_t index=0;
// 	uint32_t epfn=3;
// 	for(i=0;i<TLB_NUM;i++,vpn2++,index++,epfn++){
// 		set_cp0_entryhi(vpn2<<13|asid&0xff);
// 		set_cp0_entrylo0(epfn<<6|PTE_C<<3|PTE_D<<2|PTE_V<<1|PTE_G);
// 		epfn++;
// 		set_cp0_entrylo1(epfn<<6|PTE_C<<3|PTE_D<<2|PTE_V<<1|PTE_G);		
// 		set_cp0_pagemask(pagemask);
// 		set_cp0_index(index);
// 		asm volatile("tlbwi");
// 	}
	
// }

// static void init_memory()
// {
// 	queue_init(&emptylist);
// 	queue_init(&fulllist);
// 	TLB_flush();
// 	free_pgframe(0x01000000, 0x02000000); //init page frame
// 	//init_page_table(); 
// 	//In task1&2, page table is initialized completely with address mapping, but only virtual pages in task3.
// 	//init_TLB();		    //only used in P4 task1
// 	//init_swap();		//only used in P4 bonus: Page swap mechanism
// }

static void init_pcb()
{
	int i,j;
	queue_init(&ready_queue);
	queue_init(&sleep_queue);
	queue_init(&exit_queue);
	pcb[0].pid=0;
	pcb[0].status=TASK_RUNNING;
	int stack_top=STACK_MAX;
	queue_id=1;
	pcb[0].lock_top=-1;

	//init PCB[1]:SHELL	
	pcb[1].pid=1;
	for(j=0;j<32;j++){
			pcb[queue_id].kernel_context.regs[j]=0;
			pcb[queue_id].user_context.regs[j]=0;
	}
	pcb[queue_id].status=TASK_READY;
	pcb[queue_id].kernel_stack_top = pcb[queue_id].kernel_context.regs[29] = get_stack();

	pcb[queue_id].kernel_context.regs[31]= (uint32_t)reset_cp0;
	pcb[queue_id].kernel_context.cp0_epc = (uint32_t)test_shell;
	pcb[queue_id].kernel_context.cp0_status=0x10008003;
	pcb[queue_id].user_stack_top = pcb[queue_id].user_context.regs[29] = get_stack();

	pcb[queue_id].user_context.regs[31]=(uint32_t)test_shell;
	pcb[queue_id].user_context.cp0_epc=(uint32_t)test_shell;
	pcb[queue_id].user_context.cp0_status=0x10008003;
	pcb[queue_id].priority=100;
	pcb[queue_id].task_priority=100;
	pcb[queue_id].sleep_time=0;
	pcb[queue_id].begin_sleep_time=0;
	pcb[queue_id].lock_top=-1;
	queue_init(&pcb[queue_id].wait_queue);
	priority_queue_push(&ready_queue,(void *)&pcb[queue_id]);
	current_running=&pcb[0];
	process_id=2;
}

static void init_exception_handler()
{
	int i;
	//copy handle function to exception_handler
	exception_handler[0] = (uint32_t)handle_int;
	for(i=1;i<32;i++)
		exception_handler[i] = (uint32_t)handle_other;
	//exception_handler[2] = (uint32_t)handle_TLB;
	//exception_handler[3] = (uint32_t)handle_TLB;
	exception_handler[8] = (uint32_t)handle_syscall;
}

static void init_exception()
{
	//do_print("hello1");
	init_exception_handler();
	// 1. Get CP0_STATUS
	initial_cp0_status = GET_CP0_STATUS();

	// 2. Disable all interrupt
	// initial_cp0_status |= 0x1000ff01;
	// initial_cp0_status ^= 0x1;
	// SET_CP0_STATUS(initial_cp0_status);
	// //SET INTERRUPT ENABLE FOR FUTURE USING
	// initial_cp0_status |= 0x1;

	initial_cp0_status |= 0x10008003;
	SET_CP0_STATUS(initial_cp0_status);
	//SET INTERRUPT ENABLE FOR FUTURE USING
	initial_cp0_status = 0x10008001;
	
	
	// 3. Copy the level 2 exception handling code to 0x80000180
	bzero((void *)BEV0_EBASE, BEV0_OFFSET);
	//memcpy((void *)BEV0_EBASE, TLBexception_handler_begin, TLBexception_handler_end-TLBexception_handler_begin);
	//memcpy((void *)BEV1_EBASE, TLBexception_handler_begin, TLBexception_handler_end-TLBexception_handler_begin);
	memcpy((void *)(BEV0_EBASE+BEV0_OFFSET), exception_handler_entry, exception_handler_end-exception_handler_begin);
	memcpy((void *)(BEV1_EBASE+BEV1_OFFSET), exception_handler_entry, exception_handler_end-exception_handler_begin);

	// 4. reset CP0_COMPARE & CP0_COUNT register
	SET_CP0_COUNT(0);
	SET_CP0_COMPARE(TIMER_INTERVAL);
	
	
}

static void init_syscall(void)
{
	int i;
	for(i = 0; i < NUM_SYSCALLS; i++)
		syscall[i] = (int (*)())0;
	syscall[SYSCALL_SLEEP              ] = (int (*)()) & do_sleep;
	syscall[SYSCALL_GETPID             ] = (int (*)()) & do_getpid;
	syscall[SYSCALL_BLOCK              ] = (int (*)()) & do_block;
	syscall[SYSCALL_UNBLOCK_ONE        ] = (int (*)()) & do_unblock_one;
	syscall[SYSCALL_UNBLOCK_ALL        ] = (int (*)()) & do_unblock_all;
	syscall[SYSCALL_WRITE              ] = (int (*)()) & screen_write;
	syscall[SYSCALL_CURSOR             ] = (int (*)()) & screen_move_cursor;
	syscall[SYSCALL_REFLUSH            ] = (int (*)()) & screen_reflush;
	syscall[SYSCALL_PS                 ] = (int (*)()) & do_ps;
	syscall[SYSCALL_CLEAR              ] = (int (*)()) & do_clear;
	syscall[SYSCALL_SPAWN              ] = (int (*)()) & do_spawn;
	syscall[SYSCALL_KILL               ] = (int (*)()) & do_kill;
	syscall[SYSCALL_EXIT               ] = (int (*)()) & do_exit;
	syscall[SYSCALL_WAITPID            ] = (int (*)()) & do_wait;
	syscall[SYSCALL_MUTEX_LOCK_INIT    ] = (int (*)()) & do_mutex_lock_init;
	syscall[SYSCALL_MUTEX_LOCK_ACQUIRE ] = (int (*)()) & do_mutex_lock_acquire;
	syscall[SYSCALL_MUTEX_LOCK_RELEASE ] = (int (*)()) & do_mutex_lock_release;
	syscall[SYSCALL_BARRIER_INIT       ] = (int (*)()) & do_barrier_init;
	syscall[SYSCALL_BARRIER_WAIT       ] = (int (*)()) & do_barrier_wait; 
	syscall[SYSCALL_SEMAPHORE_INIT     ] = (int (*)()) & do_semaphore_init;
	syscall[SYSCALL_SEMAPHORE_UP       ] = (int (*)()) & do_semaphore_up;
	syscall[SYSCALL_SEMAPHORE_DOWN     ] = (int (*)()) & do_semaphore_down;
	syscall[SYSCALL_CONDITION_INIT     ] = (int (*)()) & do_condition_init;
	syscall[SYSCALL_CONDITION_WAIT     ] = (int (*)()) & do_condition_wait;
	syscall[SYSCALL_CONDITION_SIGNAL   ] = (int (*)()) & do_condition_signal;
	syscall[SYSCALL_CONDITION_BROADCAST] = (int (*)()) & do_condition_broadcast;
	syscall[SYSCALL_INIT_MAC     	   ] = (int (*)()) & do_init_mac;
	syscall[SYSCALL_NET_SEND   		   ] = (int (*)()) & do_net_send;
	syscall[SYSCALL_NET_RECV 		   ] = (int (*)()) & do_net_recv;
	syscall[SYSCALL_WAIT_RECV_PACKAGE  ] = (int (*)()) & do_wait_recv_package;
	syscall[SYSCALL_MKFS               ] = (int (*)()) & do_mkfs;
	syscall[SYSCALL_STATFS             ] = (int (*)()) & do_statfs;
	syscall[SYSCALL_MKDIR              ] = (int (*)()) & do_mkdir;
	syscall[SYSCALL_RMDIR              ] = (int (*)()) & do_rmdir;
	syscall[SYSCALL_CD                 ] = (int (*)()) & do_cd;
	syscall[SYSCALL_LS                 ] = (int (*)()) & do_ls;
	syscall[SYSCALL_TOUCH              ] = (int (*)()) & do_touch;
	syscall[SYSCALL_CAT                ] = (int (*)()) & do_cat;
	syscall[SYSCALL_FOPEN              ] = (int (*)()) & do_fopen;
	syscall[SYSCALL_FREAD              ] = (int (*)()) & do_fread;
	syscall[SYSCALL_FWRITE             ] = (int (*)()) & do_fwrite;
	syscall[SYSCALL_FCLOSE             ] = (int (*)()) & do_close;
	syscall[SYSCALL_FSEEK			   ] = (int (*)()) & do_fseek;
	syscall[SYSCALL_RENAME			   ] = (int (*)()) & do_rename;
	syscall[SYSCALL_FIND			   ] = (int (*)()) & do_find;
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

	// // init virtual memory
	// init_memory();
	// printk("> [INIT] Virtual memory initialization succeeded.\n");

	// init system call table (0_0)
	init_syscall();
	printk("> [INIT] System call initialized successfully.\n");

	// init Process Control Block (-_-!)
	init_pcb();
	printk("> [INIT] PCB initialization succeeded.\n");

	// init screen (QAQ)
	init_screen();
	printk("> [INIT] SCREEN initialization succeeded.\n");
	screen_clear(0,SCREEN_HEIGHT);

	if(init_fs() == 0)
		printk("> [INIT] No File System.\n");	
	else
		printk("> [INIT] File System initialization succeeded.\n");
	
	// TODO Enable interrupt
	SET_CP0_STATUS(initial_cp0_status);

	//init time
	time_elapsed = 0;
	
	while (1)
	{
		// (QAQQQQQQQQQQQ)
		// If you do non-preemptive scheduling, you need to use it to surrender control
		  //do_scheduler();
	};
	return;
}
