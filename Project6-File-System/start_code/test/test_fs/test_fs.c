#include "fs.h"
#include "stdio.h"
#include "string.h"
#include "test_fs.h"
#include "syscall.h"
#include "sched.h"
#include "screen.h"

static char buff[64];

void test_fs(void)
{
    int i, j;
    sys_move_cursor(0,1);
    printf("> [TASK] Testing read and write file ... \n");
    
    int fd = sys_fopen("1.txt", O_RDWR);
    sys_fseek(fd,0,0);

    for (i = 0; i < 10; i++)
    {
        sys_fwrite(fd, "hello world!\n", 13);
    }
    sys_fseek(fd,0,0);
    int print_location = 2;

    for (i = 0; i < 10; i++)
    {
        sys_fread(fd, buff, 13);
        for (j = 0; j < 13; j++)
        {
            sys_move_cursor(j,print_location+i);
            printf("%c", buff[j]);
            //sys_sleep(1);
        }
    }

    sys_close(fd);
    sys_exit();
}