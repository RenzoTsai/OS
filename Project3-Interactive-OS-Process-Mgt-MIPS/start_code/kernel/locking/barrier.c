#include "barrier.h"

void do_barrier_init(barrier_t *barrier, int goal)
{
	barrier->barrier_num=goal;
	barrier->waiting_num=0;
	queue_init(&barrier->block_queue);
}

void do_barrier_wait(barrier_t *barrier)
{
	barrier->waiting_num++;
	if(barrier->barrier_num>barrier->waiting_num){
		do_block(&barrier->block_queue);
	}
	else{
		do_unblock_all(&barrier->block_queue);
		do_scheduler();
	}
}
