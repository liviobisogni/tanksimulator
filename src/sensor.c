//*****************************************************************************
//******************** SENSOR.C - Measure tanks water level *******************
//********************        Author: Livio Bisogni         *******************
//*****************************************************************************

/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
                                  INSTRUCTIONS

    Please read the attached `README.md` file.
_____________________________________________________________________________*/


#include "sensor.h"
#include <allegro.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "init.h"
#include "graphics.h"
#include "easy_pthread_task.h"
#include "easy_pthread_time.h"


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    READ_SENSOR:    Return the distance of the first azure pixel (water color)
                    detected from (x_0, y_0) along direction alpha
_____________________________________________________________________________*/

int read_sensor(int x_0, int y_0, double alpha, double height)
{
    int c;         // pixel color value
    int x, y;      // sensor coordinates                                [pixel]
    int d;         // sensor computed distance                          [pixel]
    int height_p;  // tank height                                       [pixel]

    height_p = m2p(height);
    height_p++;  // Increse tank heght by 1 pixel for computational reasons

    d = m2p(SENSOR_MIN); /* Initialize the sensor computed distance to its
                            lowest possible value*/

    do {
        x = x_0 + d * cos(alpha);
        y = y_0 + d * sin(alpha);
        c = _getpixel32(screen, x, y); /* Faster inline versions of getpixel()
                                          for the 32 bit color depth */
        d = d + SENSOR_STEP;
    } while ((d <= height_p) && (c != AZURE));

    return d;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    UPDATE_SENSOR:  Retrieve and store the current (k = 0) measured water level
                    for the j-th task (i.e. the sensor of the i-th tank)
_____________________________________________________________________________*/

void update_sensor(int j)
{
    int    i;         // tank/sensor index (whereas j is the task index)
    int    x_0, y_0;  // sensor coordinates                             [pixel]
    double alpha;     // sensing direction                              [rad]
    int    x, y;      // tank coordinates                               [pixel]
    double r;         // tank radius                                    [m]
    double height;    // tank height                                    [m]
    double h_meas0;   // tank current (k = 0) measured water level      [m]

    i = j - NUM_TANKS;

    pthread_mutex_lock(&mux_dim);
    height = dim.height;
    r      = dim.r;
    pthread_mutex_unlock(&mux_dim);

    pthread_mutex_lock(&mux[i]);
    x = tank[i].pos.x;
    y = tank[i].pos.y;
    pthread_mutex_unlock(&mux[i]);

    // Position the distance sensor at the top-left of the tank
    x_0   = x - m2p(r) + 1;
    y_0   = y - m2p(height);
    alpha = PI / 2;  // 90 degrees, i.e., pointing downwards

    h_meas0 = height - p2m(read_sensor(x_0, y_0, alpha, height) - 2);

    if (h_meas0 > height)
        h_meas0 = height;
    if (h_meas0 < h_meas0)
        h_meas0 = 0;

    pthread_mutex_lock(&mux[i]);
    tank[i].lev.h_meas0 = h_meas0;
    pthread_mutex_unlock(&mux[i]);

    return;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    SENSOR_TASK:    Body of i-th sensor task
_____________________________________________________________________________*/

void *sensor_task(void *arg)
{
    int             i;         // tank index
    int             j;         // task index (i-th sensor task)
    struct timespec t1;        // t1:   time before task execution
    struct timespec t2;        // t2:   time after task execution
    double          rt_value;  // response_time[j] = t2 - t1                [ms]
    unsigned long   m;         // task execution counter
    char            s[50];     // for printing task name on the terminal

    m = 0;
    j = task_get_index(arg);
    i = j - NUM_TANKS;
    task_set_activation(j);

    // Measure i-th tank water level
    while (!end) {
        clock_gettime(CLOCK_MONOTONIC, &t1);  // t1

        update_sensor(j);

        clock_gettime(CLOCK_MONOTONIC, &t2);       // t2
        rt_value = get_time_diff_in_ms(&t2, &t1);  // t2 - t1
        manage_rt(j, m, rt_value);
        m++;

        task_check_deadline_miss(j);
        task_wait_for_period(j);
    }

    if (WOET == ON) {  // Print (some of the) response times on terminal
        sprintf(s, "************  Sensor[%i] Task  ************\n", i + 1);
        print_dm_rt(j, s);
    }

    return 0;
}
//-----------------------------------------------------------------------------