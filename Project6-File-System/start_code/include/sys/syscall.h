/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                       System call related processing
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
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

#ifndef INCLUDE_SYSCALL_H_
#define INCLUDE_SYSCALL_H_

#include "type.h"
#include "sync.h"
#include "sched.h"
#include "queue.h"

#define IGNORE 0
#define NUM_SYSCALLS 64

/* define */
#define SYSCALL_SLEEP 2
#define SYSCALL_PS 	3
#define SYSCALL_CLEAR 4
#define SYSCALL_SPAWN 5
#define SYSCALL_KILL 6
#define SYSCALL_EXIT 7
#define SYSCALL_WAITPID 8
#define SYSCALL_GETPID 9

#define SYSCALL_BLOCK 10
#define SYSCALL_UNBLOCK_ONE 11
#define SYSCALL_UNBLOCK_ALL 12

#define SYSCALL_BARRIER_INIT 13
#define SYSCALL_BARRIER_WAIT 14
#define SYSCALL_SEMAPHORE_INIT 15
#define SYSCALL_SEMAPHORE_UP 16
#define SYSCALL_SEMAPHORE_DOWN 17
#define SYSCALL_CONDITION_INIT 18
#define SYSCALL_CONDITION_WAIT 19

#define SYSCALL_WRITE 20
#define SYSCALL_READ 21
#define SYSCALL_CURSOR 22
#define SYSCALL_REFLUSH 23

#define SYSCALL_CONDITION_SIGNAL 24
#define SYSCALL_CONDITION_BROADCAST 25

#define SYSCALL_MUTEX_LOCK_INIT 30
#define SYSCALL_MUTEX_LOCK_ACQUIRE 31
#define SYSCALL_MUTEX_LOCK_RELEASE 32

#define SYSCALL_INIT_MAC 33
#define SYSCALL_NET_RECV 34
#define SYSCALL_NET_SEND 35
#define SYSCALL_WAIT_RECV_PACKAGE 36
#define SYSCALL_MKFS 37
#define SYSCALL_STATFS 38
#define SYSCALL_CD 39
#define SYSCALL_MKDIR 40
#define SYSCALL_RMDIR 41
#define SYSCALL_LS 42
#define SYSCALL_TOUCH 43
#define SYSCALL_CAT 44
#define SYSCALL_FOPEN 45
#define SYSCALL_FREAD 46
#define SYSCALL_FWRITE 47
#define SYSCALL_FCLOSE 48 
#define SYSCALL_FSEEK  49 
#define SYSCALL_RENAME 50
#define SYSCALL_FIND 51
/* syscall function pointer */
int (*syscall[NUM_SYSCALLS])();

void system_call_helper(int, int, int, int);
extern int invoke_syscall(int, int, int, int);

void sys_sleep(uint32_t);

void sys_block(queue_t *);
void sys_unblock_one(queue_t *);
void sys_unblock_all(queue_t *);

void sys_write(char *);
void sys_move_cursor(int, int);
void sys_reflush();

void sys_pc();
void sys_clear();
void sys_spawn(task_info_t *task);
void sys_kill(pid_t pid);
void sys_exit();
void sys_waitpid(pid_t pid);
pid_t sys_getpid ();


void sys_init_mac();
uint32_t sys_net_recv(uint32_t rd, uint32_t rd_phy, uint32_t daddr);
void sys_net_send(uint32_t td, uint32_t td_phy);

void mutex_lock_init(mutex_lock_t *);
void mutex_lock_acquire(mutex_lock_t *);
void mutex_lock_release(mutex_lock_t *);

void barrier_init(barrier_t *barrier, int goal);
void barrier_wait(barrier_t *barrier);

void semaphore_init(semaphore_t *s, int val);
void semaphore_up(semaphore_t *s);
void semaphore_down(semaphore_t *s);

void condition_init(condition_t *condition);
void condition_wait(mutex_lock_t *lock, condition_t *condition);
void condition_signal(condition_t *condition);
void condition_broadcast(condition_t *condition);

void sys_wait_recv_package();


int sys_mkfs();
int sys_mkdir(char *sname);
int sys_rmdir(char *sname);
int sys_cd(char *dir);
void sys_statfs();
int sys_ls(char *dir);
int sys_touch(char *sname);
int sys_cat(char *sname);
int sys_fopen(char *sname, int access);
int sys_fread(int fd, char *buff, int size);
int sys_fwrite(int fd, char *buff, int size);
void sys_close(int fd);
void sys_fseek(int fd, int offset, int pos);
int sys_rename(char *sname, char *new_name);
int sys_find(char * path,char * name);

#endif