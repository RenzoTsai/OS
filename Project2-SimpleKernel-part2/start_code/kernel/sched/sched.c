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
    pcb_t *p,*q;
    uint32_t current_time;
    p= sleep_queue.head;
    while(p!=NULL){
        current_time = get_timer();
        if(current_time - p->begin_sleep_time >= p->sleep_time){
            p->sleep_time=0;
            p->status=TASK_READY;
            q=queue_remove(&sleep_queue,p);
            priority_queue_push(&ready_queue,p);
            p=q;
        }
        else
            p=p->next;
    }
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
    pcb_t *p;

    //check_sleeping();
    if(current_running->status != TASK_BLOCKED ){
        current_running->status = TASK_READY;
        if(current_running->pid != 1){
            priority_queue_push(&ready_queue, current_running);
        }
    }
    current_running = (queue_is_empty(&ready_queue))?current_running:(pcb_t *)queue_dequeue(&ready_queue);
    current_running->priority = current_running->task_priority;
    current_running->status = TASK_RUNNING;

    p = (pcb_t *)ready_queue.head;
    while(p != NULL){
        p->priority ++;
        p = p->next;
    }
}


void do_sleep(uint32_t sleep_time)
{
    current_running->begin_sleep_time=get_timer();
    current_running->sleep_time=sleep_time;
    current_running->status=TASK_BLOCKED;
    queue_push(&sleep_queue,current_running);
    do_scheduler();
    // TODO sleep(seconds)
}

void do_block(queue_t *queue)
{
	current_running->status=TASK_BLOCKED;
	queue_push(queue,(void*)current_running);
	do_scheduler();
    // block the current_running task into the queue
}

// void do_unblock_one(queue_t *queue)
// {
// 	pcb_t *unblocked_task;
// 	unblocked_task=queue_dequeue(queue);
// 	unblocked_task->status=TASK_READY;
// 	queue_push(&ready_queue,unblocked_task);
// 	//printk("unblock");
//     // unblock the head task from the queue
// }

void do_unblock_one(queue_t *queue)
{
    // unblock the head task from the queue
    pcb_t *p = (pcb_t *)(queue->head);
    while(p != NULL)
    {
        p->priority += 1;
        p = p->next;
    }

    pcb_t *item = (pcb_t *)queue_dequeue(queue);
    item->status = TASK_READY;
    priority_queue_push(&ready_queue, item);
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
