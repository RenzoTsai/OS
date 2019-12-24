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
int my_pid = 2;

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
            p->which_queue=&ready_queue;
            p=q;
        }
        else
            p=p->next;
    }
}

void scheduler(void)
{
    pcb_t *position;
    pcb_t *next_running;
    check_sleeping();
    
    next_running =(queue_is_empty(&ready_queue))?current_running:(pcb_t *)queue_dequeue(&ready_queue);
    if(current_running->status != TASK_BLOCKED && current_running->status != TASK_EXITED  && next_running != current_running){
        current_running->status = TASK_READY;
        if(current_running->pid != 0){
            priority_queue_push(&ready_queue, current_running);
            current_running->which_queue=&ready_queue;
        }
    }
    current_running = next_running;
    current_running->priority = current_running->task_priority;
    current_running->status = TASK_RUNNING;
    current_running->which_queue=NULL;


    position = (pcb_t *)ready_queue.head;
    while(position != NULL){
        position->priority ++;
        position = position->next;
    }
}


void do_sleep(uint32_t sleep_time)
{
    current_running->begin_sleep_time=get_timer();
    current_running->sleep_time=sleep_time;
    current_running->status=TASK_BLOCKED;
    queue_push(&sleep_queue,current_running);
    current_running->which_queue=&sleep_queue;
    do_scheduler();
    // TODO sleep(seconds)
}

void do_block(queue_t *queue)
{
    // block the current_running task into the queue
	current_running->status=TASK_BLOCKED;
    current_running->which_queue=queue;
	queue_push(queue,(void*)current_running);
	//do_scheduler();
    
}

void do_unblock_one(queue_t *queue)
{
    // unblock the head task from the queue
    pcb_t *p = (pcb_t *)(queue->head);
    while(p != NULL){
        p->priority += 1;
        p = p->next;
    }
    pcb_t *item = (pcb_t *)queue_dequeue(queue);
    item->status = TASK_READY;
    item->which_queue = & ready_queue;
    priority_queue_push(&ready_queue, item);
}


void do_unblock_all(queue_t *queue)
{
    // unblock all task in the queue
	pcb_t *unblocked_task;
	while(!queue_is_empty(queue)){
		unblocked_task=queue_dequeue(queue);
		unblocked_task->status=TASK_READY;
        unblocked_task->which_queue=&ready_queue;
		priority_queue_push(&ready_queue,unblocked_task);
	}
}

int get_stack(){
    int new_stack;
    if(reuse_stack_top>=0)
        new_stack = reuse_stack[reuse_stack_top--];
    else{
        new_stack=stack_top ;
        stack_top-=- STACK_SIZE;
    }

    return new_stack;
}

void do_clear()
{
    screen_clear(0, SCREEN_HEIGHT);
    screen_move_cursor(0, SCREEN_HEIGHT / 2);
    do_print("-------------------- COMMAND --------------------\n");
}

void do_ps(void){
    pcb_t *p,*q;
    p= (pcb_t *) ready_queue.head;
    do_print("[PROCESS TABLE]\n");
    int i=0,j=0;
    for(j=0;j<process_id;j++){
        if(pcb[j].status==TASK_RUNNING){
            do_print("[%d] PID: %d STATUS: RUNNING\n",i,pcb[j].pid);
            i++;
        }
        else if(pcb[j].status==TASK_BLOCKED&&pcb[j].pid!=0){
            do_print("[%d] PID: %d STATUS: BLOCKED\n",i,pcb[j].pid);
            i++; 
        }
        else if(pcb[j].status==TASK_READY){
            do_print("[%d] PID: %d STATUS: READY\n",i,pcb[j].pid);
            i++; 
        }
    }

}

pid_t do_getpid()
{
    return current_running->pid;
}

void do_spawn(task_info_t *task){
    
    pcb_t *new_pcb;
    new_pcb = (queue_is_empty(&exit_queue))? &pcb[process_id]:queue_dequeue(&exit_queue);
    new_pcb = &pcb[process_id];
    new_pcb->pid=my_pid++;
    process_id++;
    int i;
    for(i=0;i<32;i++){
            new_pcb->kernel_context.regs[i]=0;
            new_pcb->user_context.regs[i]=0;
    }
    //new_pcb->pid = ((uint32_t)new_pcb - (uint32_t)pcb)/sizeof(pcb_t);
    new_pcb->type=task->type;
    new_pcb->status=TASK_READY;

    new_pcb->kernel_stack_top=new_pcb->kernel_context.regs[29]=get_stack();
    new_pcb->kernel_context.regs[31]= (uint32_t)reset_cp0;
    new_pcb->kernel_context.cp0_epc = task->entry_point;
    new_pcb->kernel_context.cp0_status=0x10008003;

    new_pcb->user_stack_top=new_pcb->user_context.regs[29]=get_stack();
    new_pcb->user_context.regs[31]=task->entry_point;
    new_pcb->user_context.cp0_epc=task->entry_point;
    new_pcb->user_context.cp0_status=0x10008003;

    new_pcb->priority=task->task_priority;
    new_pcb->task_priority=task->task_priority;
    new_pcb->sleep_time=0;
    new_pcb->begin_sleep_time=0;
    new_pcb->lock_top=-1;
    priority_queue_push(&ready_queue,(void *)new_pcb);
    new_pcb->which_queue = &ready_queue;
    queue_init(&new_pcb->wait_queue);

}


void do_kill(pid_t pid)
{
    int i,k;
    for(k=0;pcb[k].pid!=pid;k++);

    pcb_t *pcb_to_kill = &pcb[k];
    pcb_t *wait_task;
    
    /* remove pcb_to_kill from task queue */
    if(pcb_to_kill->which_queue != NULL && pcb_to_kill!=current_running){
        queue_remove(pcb_to_kill->which_queue, (void *)pcb_to_kill);
        pcb_to_kill->which_queue = NULL;
    }
    
    /* release wait task */
    do_unblock_all(&pcb_to_kill->wait_queue);

    reuse_stack[++reuse_stack_top] = pcb_to_kill->kernel_stack_top;
    reuse_stack[++reuse_stack_top] = pcb_to_kill->user_stack_top;
    queue_push(&exit_queue, (void *)pcb_to_kill);
    pcb_to_kill->which_queue=&exit_queue;

    /* release lock */
    for(i = 0; i <= pcb_to_kill->lock_top; i++)
        do_mutex_lock_release(pcb_to_kill->lock[i]);
    pcb_to_kill->status = TASK_EXITED;
    
    if(pcb_to_kill==current_running)
        do_scheduler();
    

}

void do_exit(){
    int i;
    pcb_t *pcb_to_exit = current_running;
    pcb_to_exit->which_queue = NULL;
    pcb_t *wait_task;

    /* release wait task */
    do_unblock_all(&pcb_to_exit->wait_queue);
    
    reuse_stack[++reuse_stack_top] = pcb_to_exit->kernel_stack_top;
    reuse_stack[++reuse_stack_top] = pcb_to_exit->user_stack_top;
    
    queue_push(&exit_queue, (void *)pcb_to_exit);
    pcb_to_exit->which_queue=&exit_queue;
    
     /* release lock */
    for(i = 0; i <= pcb_to_exit->lock_top; i++)
        do_mutex_lock_release(pcb_to_exit->lock[i]);
    pcb_to_exit->status = TASK_EXITED;

    do_scheduler();
}

void do_wait(pid_t pid)
{
    int k;
    for(k=0;pcb[k].pid!=pid;k++);
    if(pcb[k].which_queue != NULL){
        current_running->status = TASK_BLOCKED;
        current_running->which_queue = &pcb[k].wait_queue;
        queue_push(&pcb[k].wait_queue, (void *)current_running);
        do_scheduler();
    }
    
}

