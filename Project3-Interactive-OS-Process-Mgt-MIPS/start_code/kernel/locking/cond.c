#include "cond.h"
#include "lock.h"

void do_condition_init(condition_t *condition)
{
	condition->num_waiting=0;
	queue_init(&condition->wait_queue);
}

void do_condition_wait(mutex_lock_t *lock, condition_t *condition)
{
	condition->num_waiting++;
	do_block(&condition->wait_queue);
	do_mutex_lock_release(lock);
	do_scheduler();
	do_mutex_lock_acquire(lock);
}

void do_condition_signal(condition_t *condition)
{
	if(condition->num_waiting > 0){
		if(! queue_is_empty(&condition->wait_queue)){
			do_unblock_one(&condition->wait_queue);
    	}
		condition->num_waiting--;
	}
}

void do_condition_broadcast(condition_t *condition)
{
	do_unblock_all(condition->wait_queue);
}

