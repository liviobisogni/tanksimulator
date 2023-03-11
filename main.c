//*****************************************************************************
//**********************     MAIN.C - TANKS SIMULATOR     *********************
//**********************      Author: Livio Bisogni       *********************
//*****************************************************************************

/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
                                  INSTRUCTIONS

    Please read the attached `README.md` file.
_____________________________________________________________________________*/


#include <allegro.h>
#include <pthread.h>
#include <stdio.h>
#include "graphics.h"
#include "init.h"
#include "easy_pthread_task.h"
#include "easy_pthread_time.h"
#include "sensor.h"
#include "tank.h"
#include "user.h"


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    ACTIVATE_ALL_TASKS: Task which creates the entire task set. It has the
                        highest priority, hence all the tasks it creates have a
                        virtually identical activation time
_____________________________________________________________________________*/

void *activate_all_tasks(void *arg)
{
    int  j;      // task index
    int  rc;     // return code (value returned by task_create)
    char s[20];  // task name in human-readable format
    int  per;    // task period                                             [ms]
    int  dl;     // task relative deadline                                  [ms]
    int  pri;    // task priority; value in {1, ..., 99}

    pthread_mutex_lock(&mux_print);
    for (j = 0; j < NUM_TASKS; j++) {
        // TANK tasks creation
        if (j < NUM_TANKS) {
            per = T_TANKS;
            dl  = DL_TANKS;
            pri = P_TANKS;
            rc  = task_create(tank_task, j, per, dl, pri);
            sprintf(s, "tank[%i]", j);
        }
        // SENSOR tasks creation
        else if (j < 2 * NUM_TANKS) {
            per = T_SENSORS;
            dl  = DL_SENSORS;
            pri = P_SENSORS;
            rc  = task_create(sensor_task, j, per, dl, pri);
            sprintf(s, "sensor[%i]", j - NUM_TANKS);
        }
        // USER task creation
        else if (j == NUM_TANKS * 2) {
            per = T_USER;
            dl  = DL_USER;
            pri = P_USER;
            rc  = task_create(user_task, j, per, dl, pri);
            sprintf(s, "user");
        }
        // GRAPHICS task creation
        else if (j == NUM_TANKS * 2 + 1) {
            per = T_GRAPHICS;
            dl  = DL_GRAPHICS;
            pri = P_GRAPHICS;
            rc  = task_create(graphics_task, j, per, dl, pri);
            sprintf(s, "graphics");
        }

        if (rc == 0)
            printf("Task %d (%s) created and activated.\n", j, s);
        else
            printf("Error %d in creating (%s) task %d.\n", rc, s, j);
    }
    pthread_mutex_unlock(&mux_print);

    return 0;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    PRINT_SUMMARY:  Print a brief summary of the entire program
_____________________________________________________________________________*/

void print_summary(double elapsed_time)
{
    int    j;         // task index
    int    dm;        // deadline misses
    double util_tot;  // total processor utilization factor                 [ms]

    util_tot = 0;
    for (j = 0; j < NUM_TASKS; j++)
        util_tot += task_get_util_avg(j);

    pthread_mutex_lock(&mux_print);
    printf("\n\n\n\n");
    printf("\n\n\n******************************************\n");
    printf("*****         TANK SIMULATOR:        *****\n");
    printf("***** © 2021 REAL-TIME INDUSTRY Inc. *****\n");
    printf("******************************************\n");
    printf("\nTotal elapsed time:\t%f s\n", elapsed_time / 1000);
    printf("\nUtilization factor:\t%.2f %%\n\n", util_tot * PERCENT);

    printf("Deadline misses:\n");
    for (j = 0; j < 2 * NUM_TANKS + 2; j++) {
        dm = task_get_deadline_miss(j);
        if (j >= 0 && j < NUM_TANKS)
            printf("\tTask %d\t(Tank[%d]):  \t%d\n", j + 1, j + 1, dm);
        if (j >= NUM_TANKS && j < 2 * NUM_TANKS)
            printf("\tTask %d\t(Sensor[%d]):\t%d\n", j + 1, j - NUM_TANKS + 1,
                   dm);
        if (j == 2 * NUM_TANKS)
            printf("\tTask %d\t(User):\t\t%d\n", j + 1, dm);
        if (j == 2 * NUM_TANKS + 1)
            printf("\tTask %d\t(Graphics):\t%d\n", j + 1, dm);
    }
    pthread_mutex_unlock(&mux_print);

    printf("\n\nExiting from Main.\n");
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    TERMINATE_TASKS:    Terminate the entire task set
_____________________________________________________________________________*/

void terminate_tasks()
{
    int j;   // task index
    int rc;  // return code (value returned by task_wait_for_end)

    for (j = 0; j < NUM_TASKS; j++) {
        rc = task_wait_for_end(j);
        if (rc != 0)
            printf("ERROR: return code from pthread_join() is %d\n", rc);
        else {  // task j-th terminated correctly
                // printf("Task %i terminated correctly\n", j);
        }
    }
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    MAIN:   Execute the program
_____________________________________________________________________________*/

int main(void)
{
    int rc;                        /* return code (value
                                      returned by task_create) */
    struct timespec t1;            // t1:   simulator starting time
    struct timespec t2;            // t2:   simulator ending time
    double          elapsed_time;  // t2 - t1                               [ms]

    init();  // Initilize the simulator

    clock_gettime(CLOCK_MONOTONIC, &t1);  // t1

    // Create the task set (with a virtually identical activation time)
    rc = task_create(activate_all_tasks, NUM_TASKS, 1, 1, LINUX_MAX_PRIO);
    if (rc == 0) {
        printf("Entire task set created and activated successfully.\n");
        task_wait_for_end(NUM_TASKS);
    } else {
        printf("Error %d in creating the task set\n", rc);
    }

    while (!key[KEY_ESC]) {
    };

    end = 1;  // End all the tasks (==> End the simulation)

    terminate_tasks();

    allegro_exit();

    clock_gettime(CLOCK_MONOTONIC, &t2);           // t2
    elapsed_time = get_time_diff_in_ms(&t2, &t1);  // t2 - t1

    if (WOET == ON) {  // Print a brief summary on the terminal
        print_summary(elapsed_time);
    }

    return 0;
}
END_OF_MAIN(); /* This must be called right after the closing bracket of the
                  MAIN function. It is Allegro specific */
//-----------------------------------------------------------------------------