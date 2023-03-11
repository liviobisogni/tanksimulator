//*****************************************************************************
//******** INIT.C - Define global variables, initialize the simulator *********
//********          and manage real-time (using functions defined in  *********
//********          'libeasypthread.a') *********
//********                    Author: Livio Bisogni                   *********
//*****************************************************************************

/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
                                  INSTRUCTIONS

    Please read the attached `README.md` file.
_____________________________________________________________________________*/


#include "init.h"
#include <allegro.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "graphics.h"
#include "easy_pthread_task.h"
#include "tank.h"


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    GLOBAL VARIABLES (definition)
_____________________________________________________________________________*/

//-----------------------------------------------------------------------------
/* Simulator variables */
int            end;
tank_attr      tank[NUM_TANKS];
pid_controller pid;
dimension      dim;
double         k;
int            w;
double         util_scale;
//-----------------------------------------------------------------------------
/* Mutexes */
pthread_mutex_t mux[NUM_TANKS];
pthread_mutex_t mux_pid;
pthread_mutex_t mux_dim;
pthread_mutex_t mux_k;
pthread_mutex_t mux_w;
pthread_mutex_t mux_util;
pthread_mutex_t mux_print;
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    MAX:    Return the larger integer among a and b
_____________________________________________________________________________*/

int max(int a, int b)
{
    int max;

    max = a;
    if (b > max)
        max = b;

    return max;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    MANAGE_RT:  1) Store various real-time indicators for the j-th task:
                    * rt_value;         response time current value         [ms]
                    * rt_index;         response time current index         []
                    * m;                task execution counter              []
                    * rt_tot;           summation of all the response times [ms]
                2) Moreover, compute (and store):
                    * rt_max;           maximum response time               [ms]
                    * rt_min;           minimum response time               [ms]
                    * util_inst;        instantaneous utilization factor    [%]
                    * util_inst_max;    max. instantaneous utilization factor[%]
                    * util_avg;         average utilization factor          [%]
                    * rt_avg;           average response time               [ms]
                    * rt_std;           standard deviation of response time [ms]
_____________________________________________________________________________*/

void manage_rt(int j, unsigned long m, double rt_value)
{
    double rt_max;
    double rt_min;
    double util_inst;
    double util_inst_max;
    double util_avg;
    double rt_avg;
    double rt_std;

    // 1) Store
    task_set_rt_value(j, m, rt_value);
    task_set_rt_index(j, m, m);
    task_set_ex_tot(j, m);
    task_increment_rt_tot(j, rt_value);

    // 2) Compute (and store):
    rt_max = task_compute_rt_max(j);
    task_set_rt_max(j, rt_max);
    rt_min = task_compute_rt_min(j);
    task_set_rt_min(j, rt_min);
    util_inst = task_compute_util_inst(j);
    task_set_util_inst(j, util_inst);
    util_inst_max = task_compute_util_inst_max(j);
    task_set_util_inst_max(j, util_inst_max);
    util_avg = task_compute_util_avg(j);
    task_set_util_avg(j, util_avg);
    rt_avg = task_compute_rt_avg(j);
    task_set_rt_avg(j, rt_avg);
    rt_std = task_compute_std_dev(j);
    task_set_rt_std(j, rt_std);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    PRINT_N_WORST_RT:   Given two arrays - one of response time values
                        (rt_values) and the other of their corresponding indexes
                        (rt_indexes), both of length dim - find and print the n
                        largest numbers (and their corresponding indexes as
                        well).
                        Function used for printing task n worst response
                        times (rt).
                        Code adapted from:

https://stackoverflow.com/questions/27237742/how-to-find-top-6-elements-in-an-array-in-c
_____________________________________________________________________________*/

void print_n_worst_rt(double rt_values[], unsigned long rt_indexes[],
                      unsigned long dim, int n)
{
    int           i;     // index for n; value in {0, ..., n - 1}
    unsigned long j;     // index of considered number; value in {0, ..., dim}
    unsigned long max;   // index of (i+1)-th largest number (i = 0, ..., n - 1)
    double        temp;  // for swapping purposes

    // Partial selection sort, move n max elements to front
    for (i = 0; i < n; i++) {
        max = i;
        // Find next max index
        for (j = i + 1; j < dim; j++) {
            if (rt_values[j] > rt_values[max]) {
                max = j;
            }
        }
        // Swap rt_values in input array
        temp           = rt_values[i];
        rt_values[i]   = rt_values[max];
        rt_values[max] = temp;
        // Swap rt_indexes in tracking array
        temp            = rt_indexes[i];
        rt_indexes[i]   = rt_indexes[max];
        rt_indexes[max] = temp;
    }

    printf("%i worst response times:\n", n);
    for (i = 0; i < n; i++) {
        if (i < 10 - 1)
            printf("\t%i)  rt[%lu]:\t%f\n", i + 1, rt_indexes[i] + 1,
                   rt_values[i]);
        else  // Note: n should be lower than 100
            printf("\t%i) rt[%lu]:\t%f\n", i + 1, rt_indexes[i] + 1,
                   rt_values[i]);
    }
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    PRINT_DM_RM:    Print j-th task current deadline misses and some of its
                    noticeable response times on the terminal.
                    String s should be 44-character long (without counting the
                    blanks) and should have the following structure:
                            "************    Task_Name     ************\n"
_____________________________________________________________________________*/

void print_dm_rt(int j, char s[])
{
    int            dm;
    double         rt_avg, rt_max, rt_min, rt_tot, rt_std, util_avg;
    double        *rt_values;
    unsigned long *rt_indexes;
    unsigned long  m;

    /* Compute (from scratch) real maximum response time of the j-th task
       This is necessary, due to possible overwritings of rt_max */
    task_set_rt_max(j, task_compute_rt_max_from_scratch(j));
    // task_set_rt_min(j, task_compute_rt_min_from_scratch(j)); // Redundant
    // task_set_rt_avg(j, task_compute_rt_avg_from_scratch(j)); // Redundant

    dm         = task_get_deadline_miss(j);
    rt_avg     = task_get_rt_avg(j);
    rt_max     = task_get_rt_max(j);
    rt_min     = task_get_rt_min(j);
    rt_tot     = task_get_rt_tot(j);
    rt_std     = task_get_rt_std(j);
    util_avg   = task_get_util_avg(j);
    rt_values  = task_get_rt_values(j);
    rt_indexes = task_get_rt_indexes(j);
    m          = task_get_ex_tot(j) + 1;  // Add 1 cuz m counts from 0

    pthread_mutex_lock(&mux_print);
    printf("\n\n\n******************************************\n");
    printf("************ WCET ESTIMATION: ************\n");
    printf("%s", s);  // e.g., "************    Task_Name     ************\n"
    printf("******************************************\n");
    printf("Loop length: %lu cycles\n", m);  // loop length             [cycles]
    printf("Deadline misses: %i\n", dm);     // deadline misses         []
    printf("NOTE: Times are expressed in milliseconds.\n");

    printf("Response time analysis:\n");
    printf("\tAverage (AORT):\t\t%f\n", rt_avg);    // avg              [ms]
    printf("\tMaximum (WORT):\t\t%f\n", rt_max);    // max              [ms]
    printf("\tMinimum (BORT):\t\t%f\n", rt_min);    // min              [ms]
    printf("\tTotal:\t\t\t%f\n", rt_tot);           // tot              [ms]
    printf("\tStandard deviation:\t%f\n", rt_std);  // std_dev          [ms]
    printf("\tUtilization factor:\t%f %%\n",
           util_avg * PERCENT);  //                    util             [%]

    print_n_worst_rt(rt_values, rt_indexes, m, WOET_N_WORST);
    pthread_mutex_unlock(&mux_print);

    return;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INIT_SIM_PARAMS:    Initialize all the simulator parameters
_____________________________________________________________________________*/

void init_sim_params()
{
    int i;  // tank index
    int b;  /* buffer index (circular buffer, used to store/retrieve previous
               phi_in values) */
    int red, green, blue;  // primary colors; pseudo-randomly generated
    int color_rand;        // = (red, green, blue)

    init_integration_interval();

    pthread_mutex_lock(&mux_k);
    k = K_DEFAULT;
    pthread_mutex_unlock(&mux_k);
    pthread_mutex_lock(&mux_pid);
    pid.kp = KP_DEFAULT;
    pid.ki = KI_DEFAULT;
    pid.kd = KD_DEFAULT;
    pid.f  = F_DEFAULT;
    pthread_mutex_unlock(&mux_pid);
    pthread_mutex_lock(&mux_dim);
    dim.height = HEIGHT_DEFAULT;
    dim.r      = R_DEFAULT;
    pthread_mutex_unlock(&mux_dim);
    pthread_mutex_lock(&mux_w);
    w = W_DEFAULT;
    pthread_mutex_unlock(&mux_w);
    pthread_mutex_lock(&mux_util);
    util_scale = U_DEFAULT;
    pthread_mutex_unlock(&mux_util);

    // Initialize all the tanks
    for (i = 0; i < NUM_TANKS; i++) {
        pthread_mutex_lock(&mux[i]);

        tank[i].lev.h = (double)HEIGHT_MAX -
                        ((double)i * HEIGHT_MAX / max((NUM_TANKS - 1), 1));
        tank[i].lev.h_meas0 = H_EST_DEFAULT;
        tank[i].lev.h_des   = (double)i * HEIGHT_MAX / max((NUM_TANKS - 1), 1);
        tank[i].lev.h_des   = quantize(tank[i].lev.h_des);
        tank[i].lev.h_e0    = tank[i].lev.h_des - tank[i].lev.h_meas0;
        tank[i].lev.h_e1    = tank[i].lev.h_e0;

        tank[i].pos.x = TANKSIM_X + (0.5 + i) * (TANKSIM_WIDTH / 5);
        tank[i].pos.y = TANKSIM_Y + TANKSIM_HEIGHT - MAX_TANK_BORDER_THICKNESS -
                        m2p(2 * CIRCLE_RADIUS) - 1.5 * YCIRCLE_OFFSET;

        tank[i].phi.out        = PHI_OUT_DEFAULT;
        tank[i].phi.in         = PHI_IN_DEFAULT;
        tank[i].phi.phi_in_top = 0;

        // Initialize circular buffer for the i-th tank inlet valve
        for (b = 0; b < PHI_IN_MAX_BUF_LEN; b++) {
            tank[i].phi.phi_in_buffer[b] = tank[i].phi.in;
        }

        // Generate pseudo-random color for the i-th tank
        red           = rand() % 256;
        green         = rand() % 256;
        blue          = rand() % 256;
        color_rand    = makecol(red, green, blue);
        tank[i].color = color_rand;

        pthread_mutex_unlock(&mux[i]);

        store_phi_in(i); /* Store actuator action (the current phi_in value) in
                            the i-th tank circular buffer */
    }
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INIT_MUTEXES:   Initialize mutexes (for mutual exclusion of global
                    variables)
_____________________________________________________________________________*/

void init_mutexes()
{
    int                 i;     // tank index
    pthread_mutexattr_t matt;  // for defining mutex attributes

    pthread_mutexattr_init(&matt);

    // Use the Priority Inheritance Protocol (PIP)
    pthread_mutexattr_setprotocol(&matt, PTHREAD_PRIO_INHERIT);

    for (i = 0; i < NUM_TANKS; ++i) {
        pthread_mutex_init(&mux[i], &matt);
    }
    pthread_mutex_init(&mux_pid, &matt);
    pthread_mutex_init(&mux_dim, &matt);
    pthread_mutex_init(&mux_k, &matt);
    pthread_mutex_init(&mux_w, &matt);
    pthread_mutex_init(&mux_util, &matt);
    pthread_mutex_init(&mux_print, &matt);

    pthread_mutexattr_destroy(&matt);  // Destroy mutex attributes
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INIT_GRAPHICS:  Initialize graphics
_____________________________________________________________________________*/

void init_graphics()
{
    allegro_init();

    set_color_depth(BITS);
    if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, RES_X, RES_Y, 0, 0) != 0)
        printf("Cannot set graphic mode!!!");

    install_timer();
    install_keyboard();
    install_mouse();
    enable_hardware_cursor(); /* Use the operating system's native mouse
                                 cursor rather than some custom cursor */
    show_mouse(screen);

    init_graphics_buffers();

    // 1) Static graphics (to be drawn once)
    draw_static_graphics();

    reset_util_inst_max_process();
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INIT:   Initialize the simulator (ending flag, randomness seed, graphics,
            simulator parameters, and mutexes)
_____________________________________________________________________________*/

void init()
{
    end = 0;  // Initialize end flag (==> Start the simulation)

    /* Seed the random number generator with the current time.
       It should be called once (and only once) prior to calling any other
       random number function */
    srand(time(NULL));

    init_graphics();

    init_mutexes();

    init_sim_params();
}
//-----------------------------------------------------------------------------