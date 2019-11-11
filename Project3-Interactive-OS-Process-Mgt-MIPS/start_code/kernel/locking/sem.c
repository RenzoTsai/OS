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
		do_unblock_one(&s->block_queue);
    }	
}

void do_semaphore_down(semaphore_t *s)
{
	s->sem--;
	if(s->sem<0){
		do_block(&s->block_queue);
		do_scheduler();
	}
}
