//*****************************************************************************
//********** EASY_PTHREAD_TASK.H - Header file of easy_pthread_task.c *********
//**********                  Author: Livio Bisogni                   *********
//*****************************************************************************

#ifndef EASY_PTHREAD_TASK_H
#define EASY_PTHREAD_TASK_H

#include <pthread.h>
#include "easy_pthread_time.h"


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    GLOBAL CONSTANTS
_____________________________________________________________________________*/
#define NT     20          // maximum possible number of tasks
#define GOOGOL 10000000    // maximum possible length of response time arrays
                           /* e.g.:	GOOGOL = 10000000 and task_period = 20 ms
                                    can guarantee more than 55 hours of response
                                    time data recording */
#define LINUX_MAX_PRIO 99  // highest priority level allowed in Linux		[]
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    STRUCTURES DEFINITION
_____________________________________________________________________________*/

//-----------------------------------------------------------------------------
/* Real-Time parameters */
/* Such a structure must be initialized before calling the thread_create and
   passed as a thread argument. A structure for each thread is required. */
struct task_par
{
    int            ind;        // task index; value in {0, ..., NT - 1}
    int            per;        // task period								[ms]
    int            dl_r;       // relative deadline							[ms]
    int            pri;        // task priority; value in {1, ..., 99}
    int            dm;         // number of deadline misses
    double         rt_avg;     // average response time						[ms]
    double         rt_max;     // maximum response time						[ms]
    double         rt_min;     // minimum response time						[ms]
    double         rt_tot;     // total response time						[ms]
    double         rt_std;     // standard deviation of the response time	[ms]
    double         util_inst;  // instantaneous utilization factor
    double         util_inst_max;  // maximumm instantaneous utilization factor
    double         util_avg;       // average utilization factor
    double        *rt_values;      // array storing all the response times	[ms]
    unsigned long *rt_indexes;     // array storing the indexes of the rt
    unsigned long  ex_tot;         /* (current) total number of task execution;
                                      value in {0, ..., GOOGOL - 1}.
                                      Note: it starts counting from 0, not 1;
                                      hence, the actual number of execution is
                                      (ex_tot + 1) */
    struct timespec at;            // next activation time
    struct timespec dl_a;          // absolute deadline
};
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    FUNCTION PROTOTYPES
_____________________________________________________________________________*/

int task_create(void *(*task)(void *), int j, int per, int drel, int prio);

int task_get_index(void *ind);

int task_get_period(int j);

int task_get_max_priority();

int task_get_max_preriod();

void task_set_activation(int j);

int task_check_deadline_miss(int j);

int task_get_deadline_miss(int j);

int task_set_deadline_miss(int j, int dm);

void task_wait_for_period(int j);

int task_wait_for_end(int j);

double task_get_rt_avg(int j);

double task_get_rt_max(int j);

double task_get_rt_min(int j);

double task_get_rt_tot(int j);

double task_get_rt_std(int j);

double task_get_util_inst(int j);

double task_get_util_inst_max(int j);

double task_get_util_avg(int j);

double *task_get_rt_values(int j);

unsigned long *task_get_rt_indexes(int j);

double task_get_rt_value(int j, unsigned long m);

unsigned long task_get_rt_index(int j, unsigned long m);

unsigned long task_get_ex_tot(int j);

void task_set_rt_avg(int j, double rt_avg);

void task_set_rt_max(int j, double rt_max);

void task_set_rt_min(int j, double rt_min);

void task_set_rt_tot(int j, double rt_tot);

void task_increment_rt_tot(int j, double rt_value);

void task_set_rt_std(int j, double rt_std);

void task_set_util_inst(int j, double util_inst);

void task_set_util_inst_max(int j, double util_inst_max);

void task_set_util_avg(int j, double util_avg);

void task_set_rt_values(int j, double rt_values[], unsigned long dim);

void task_set_rt_indexes(int j, unsigned long rt_indexes[], unsigned long dim);

void task_set_rt_value(int j, unsigned long m, double rt_values);

void task_set_rt_index(int j, unsigned long m, unsigned long rt_indexes);

void task_set_ex_tot(int j, unsigned long ex_tot);

double task_compute_rt_max(int j);

double task_compute_rt_max_from_scratch(int j);

double task_compute_rt_min(int j);

double task_compute_rt_min_from_scratch(int j);

double task_compute_std_dev(int j);

double task_compute_util_inst(int j);

double task_compute_util_inst_max(int j);

double task_compute_util_avg(int j);

double task_compute_rt_avg(int j);

double task_compute_rt_avg_from_scratch(int j);

//-----------------------------------------------------------------------------


#endif  // EASY_PTHREAD_TASK_H