#include "sched.h"
#include "stdio.h"
#include "test2.h"
#include "syscall.h"
#include "screen.h"

static char blank[] = {"                    "};
static char plane1[] = {"    ___         _   "};
static char plane2[] = {"| __\\_\\______/_| ....This is an Easter Eggs "};
static char plane3[] = {"<[___\\_\\_______|  "};
static char plane4[] = {"|  o'o              "};

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

void printf_task1(void)
{
    int i;
    int print_location = 1;

    for (i = 0;; i++)
    {
        disable_interrupt();
        sys_move_cursor(1, print_location);
        printf("> [TASK] This task is to test scheduler. (%d)", i);
        enable_interrupt();
    }
}

void printf_task2(void)
{
    int i;
    int print_location = 2;

    for (i = 0;; i++)
    {
        disable_interrupt();
        sys_move_cursor(1, print_location);
        printf("> [TASK] This task is to test scheduler. (%d)", i);
        enable_interrupt();
    }
}

void drawing_task2(void)
{
    int i, j = SCREEN_HEIGHT / 2 - 9;
    int k=55;

    while (k>0)
    {
        for (i = 55; i > 0; i--)
        {
            disable_interrupt();
            sys_move_cursor(i, j + 0);
            printf("%s", plane1);

            sys_move_cursor(i, j + 1);
            printf("%s", plane2);

            sys_move_cursor(i, j + 2);
            printf("%s", plane3);

            sys_move_cursor(i, j + 3);
            printf("%s", plane4);
            enable_interrupt();
        }
        disable_interrupt();
        sys_move_cursor(1, j + 0);
        printf("%s", blank);

        sys_move_cursor(1, j + 1);
        printf("%s", blank);

        sys_move_cursor(1, j + 2);
        printf("%s", blank);

        sys_move_cursor(1, j + 3);
        printf("%s", blank);
        enable_interrupt();
        k--;
    }
    sys_clear();

}