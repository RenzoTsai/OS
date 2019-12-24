/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode. 
 *       The main function is to make system calls through the user's output.
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

#include "test.h"
#include "stdio.h"
#include "screen.h"
#include "syscall.h"
#include "sched.h"
#include "fs.h"
#define MAXLEN 100

static void disable_interrupt()
{
    uint32_t cp0_status = GET_CP0_STATUS();
    cp0_status &= 0xfffffffe;
    SET_CP0_STATUS(cp0_status);
}

static void enable_interrupt()
{
    uint32_t cp0_status = GET_CP0_STATUS();
    cp0_status |= 0x01;
    SET_CP0_STATUS(cp0_status);
}

static char read_uart_ch(void)
{
    char ch = 0;
    unsigned char *read_port = (unsigned char *)(0xbfe48000 + 0x00);
    unsigned char *stat_port = (unsigned char *)(0xbfe48000 + 0x05);

    while ((*stat_port & 0x01))
    {
        ch = *read_port;
    }
    return ch;
}

//Running project_4 from shell is recommended. You can also run it from loadboot.
struct task_info task1 = {"task1", (uint32_t)&test_fs, USER_PROCESS};
// struct task_info task2 = {"task2", (uint32_t)&mac_send_task, USER_PROCESS};
// struct task_info task3 = {"task3", (uint32_t)&mac_recv_task, USER_PROCESS};
struct task_info *test_tasks[1] = {&task1};

int num_test_tasks = 3;

void process_cmd(uint32_t argc, char argv[6][15])
{
    if(argc == 1){
        if(!strcmp(argv[0], "ps"))
        {
            sys_ps();
        }
        else if(!strcmp(argv[0], "cd"))
        {
            sys_sleep(1);
            argv[1][0] = '\0';
            sys_cd((char *)argv[1]);
        }
        else if(!strcmp(argv[0], "ls"))
        {
            sys_sleep(1);
            argv[1][0] = '\0';
            sys_ls((char *)argv[1]);
           // printf("Successed! cur_inum = %d\n", cur_inode->inum);
        }
        else if(!strcmp(argv[0], "clear"))
            sys_clear();
        else if(!strcmp(argv[0], "exit"))
            sys_exit();
        else if(!strcmp(argv[0], "mkfs"))
            sys_mkfs();
        else if(!strcmp(argv[0], "statfs"))
            sys_statfs();
        else
            printf("Unknown command!\n");
    }
    else if(argc == 2){
        int pid = atoi((char *)argv[1]);
        if(!strcmp(argv[0], "exec"))
        {
            sys_spawn(test_tasks[pid-1]);
            printf("exec process[%d]\n", pid);
        }
        else if(!strcmp(argv[0], "kill"))
        {
            sys_kill(pid);
            //printf("kill process pid = %d\n", pid);
        }
        else if(!strcmp(argv[0], "mkdir"))
        {
            sys_sleep(1);
            sys_mkdir((char *)argv[1]);
            //printf("Successed! cur_inum = %d\n", cur_inode->inum);
        }
        else if(!strcmp(argv[0], "rmdir"))
        {
            sys_rmdir((char *)argv[1]);
            //printf("Successed! cur_inum = %d\n", cur_inode->inum);
        }
        else if(!strcmp(argv[0], "touch"))
        {
            sys_touch((char *)argv[1]);
            //printf("Successed! cur_inum = %d\n", cur_inode->inum);
        }
        else if(!strcmp(argv[0], "cd"))
        {
            sys_sleep(1);
            sys_cd((char *)argv[1]);
            //printf("Successed! cur_inum = %d\n", cur_inode->inum);
        }
        else if(!strcmp(argv[0], "ls"))
        {
            sys_sleep(1);
            sys_ls((char *)argv[1]);
            //printf("Successed! cur_inum = %d\n", cur_inode->inum);
        }
        else if(!strcmp(argv[0], "cat"))
        {
            sys_cat((char *)argv[1]);
            //printf("Successed! cur_inum = %d\n", cur_inode->inum);
        }
        else if(!strcmp(argv[0], "kill"))
        {
            sys_kill(pid);
            //printf("kill process pid = %d\n", pid);
        }
        else
            printf("Unknown command!\n");
    }
    else if(argc != 0)
        printf("Unknown command! [argc =%d]\n",argc);
}

void test_shell()
{
    char cmd[80];
    uint32_t i = 0, argc, j, k;
    char argv[6][15];

    /* terminal */
    sys_move_cursor(0, SCREEN_HEIGHT / 2);
    printf("-------------------- COMMAND --------------------\n");
    printf("> root@Vincent OS: ");
    
    while (1)
    {
        // read command from UART port
        disable_interrupt();
        char ch = read_uart_ch();
        enable_interrupt();

        // TODO solve command
        if(ch == 0 || (i == 0 && (ch == 0x7f || ch == 8)))
            continue;
        printf("%c", ch);
        if(ch != '\r')
        {
            if(ch == 8 || ch == 0x7f){ // BS
                i--;
                // screen_cursor_x-=2;   //clear invalid char in QEMU
                // printf(" ");
                // screen_cursor_x--;
            }
            else 
                cmd[i++] = ch;
            continue;
        }
        else
        {
            cmd[i++] = ' ', cmd[i] = '\0';
            j = 0;
            if(cmd[j] == ' ')
                while(cmd[j] == ' ' && j < i)
                    j++;
            for(argc = k = 0; j < i; j++)
            {
                if(cmd[j] == ' ')
                {
                    argv[argc][k] = '\0';
                    argc++, k = 0;
                    while(cmd[j] == ' ' && j < i)
                        j++;
                }
                argv[argc][k++] = cmd[j];
            }

            process_cmd(argc, argv);

            i = 0;
            printf("> root@Vincent OS: ");
        }
    }
}