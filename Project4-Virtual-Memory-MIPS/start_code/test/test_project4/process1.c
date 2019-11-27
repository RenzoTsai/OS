#include "sched.h"
#include "stdio.h"
#include "syscall.h"
#include "screen.h"
#include "test4.h"

static char blank[] = {"                   "};
static char plane1[] = {"    ___         _  "};
static char plane2[] = {"| __\\_\\______/_| "};
static char plane3[] = {"<[___\\_\\_______| "};
static char plane4[] = {"|  o'o             "};

void drawing_task1(void)
{
    int i = 22, j = 10;

    while (1)
    {
        for (i = 60; i > 0; i--)
        {
            /* move */
            vt100_move_cursor(i, j + 0);
            printf("%s", plane1);

            vt100_move_cursor(i, j + 1);
            printf("%s", plane2);

            vt100_move_cursor(i, j + 2);
            printf("%s", plane3);

            vt100_move_cursor(i, j + 3);
            printf("%s", plane4);
        }

        vt100_move_cursor(1, j + 0);
        printf("%s", blank);

        vt100_move_cursor(1, j + 1);
        printf("%s", blank);

        vt100_move_cursor(1, j + 2);
        printf("%s", blank);

        vt100_move_cursor(1, j + 3);
        printf("%s", blank);
    }
}
