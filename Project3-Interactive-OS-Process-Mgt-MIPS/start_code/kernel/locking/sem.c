#include "sem.h"
#include "stdio.h"
#include "sched.h"

void do_semaphore_init(semaphore_t *s, int val)
{
	s->sem =val;
	queue_init(&s->block_queue);
}

void do_semaphore_up(semaphore_t *s)
{
	s->sem++;
	if(s->sem<=0){
        pcb_t *item = (pcb_t *)queue_dequeue(&s->block_queue);
        item->status = TASK_READY;
        item->which_queue = &ready_queue;
        priority_queue_push(&ready_queue, (void *)item);
    }	
}

void do_semaphore_down(semaphore_t *s)
{
	if(s>0)
		s->sem--;
	else{
		do_block(&s->block_queue);
		do_scheduler();
	}
}
