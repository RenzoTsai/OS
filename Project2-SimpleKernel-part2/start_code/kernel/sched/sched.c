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

// void scheduler(void)
// {

// 	if(current_running->status!=TASK_BLOCKED){
// 		current_running->status=TASK_READY;
// 		if(current_running->pid!=1){
// 			queue_push(&ready_queue,current_running);
// 		}
// 	}
// 	if(!queue_is_empty(&ready_queue))
// 		current_running=(pcb_t *)queue_dequeue(&ready_queue);

// 	current_running->status=TASK_RUNNING;
// }

void scheduler(void)
{
    pcb_t *next_running, *p;
    if(queue_is_empty(&ready_queue))
        next_running = current_running;
    else
        next_running = (pcb_t *)queue_dequeue(&ready_queue);

    if(current_running->status != TASK_BLOCKED && next_running != current_running)
    {
        current_running->status = TASK_READY;
        if(current_running->pid != 1)
        {
            priority_queue_push(&ready_queue, current_running);
        }
    }
    current_running = next_running;
    current_running->priority = current_running->task_priority;
    current_running->status = TASK_RUNNING;

    p = (pcb_t *)ready_queue.head;
    while(p != NULL)
    {
        p->priority += 1;
        p = p->next;
    }
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
	//printk("block");
    // block the current_running task into the queue
}

void do_unblock_one(queue_t *queue)
{
	pcb_t *unblocked_task;
	unblocked_task=queue_dequeue(queue);
	unblocked_task->status=TASK_READY;
	queue_push(&ready_queue,unblocked_task);
	//printk("unblock");
    // unblock the head task from the queue
}

void do_unblock_all(queue_t *queue)
{
	pcb_t *unblocked_task;
	while(!queue_is_empty(queue)){
		unblocked_task=queue_dequeue(queue);
		unblocked_task->status=TASK_READY;
		queue_push(&ready_queue,unblocked_task);
	}

    // unblock all task in the queue
}
