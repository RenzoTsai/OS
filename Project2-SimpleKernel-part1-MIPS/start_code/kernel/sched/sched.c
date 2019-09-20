#include "lock.h"
#include "time.h"
#include "stdio.h"
#include "sched.h"
#include "queue.h"
#include "screen.h"

pcb_t pcb[NUM_MAX_TASK];

/* current running task PCB */
pcb_t *current_running;

/* global process id */
pid_t process_id = 1;

static void check_sleeping()
{
}

void scheduler(void)
{
	pcb_t *next_running;
	if(!queue_is_empty(&ready_queue))
		next_running=(pcb_t *)queue_dequeue(&ready_queue);
	else
		next_running=current_running;

	if(current_running->status!=TASK_BLOCKED){
		current_running->status=TASK_READY;
		if(current_running->pid!=1)
			queue_push(&ready_queue,current_running);
	}
	
	
	current_running=next_running;
	current_running->status=TASK_RUNNING;
	//printk("changed\n");

	
	//current_running->status=TASK_RUNNING;
    // Modify the current_running pointer.
}

void do_sleep(uint32_t sleep_time)
{
    // TODO sleep(seconds)
}

void do_block(queue_t *queue)
{
	current_running->status=TASK_BLOCKED;
	queue_push(queue,(void*)current_running);
	do_scheduler();
    // block the current_running task into the queue
}

void do_unblock_one(queue_t *queue)
{
    // unblock the head task from the queue
}

void do_unblock_all(queue_t *queue)
{
    // unblock all task in the queue
}
