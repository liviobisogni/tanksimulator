//*****************************************************************************
//**************** TANK.C - Compute tanks real water level and  ***************
//****************          control their inlet valve positions ***************
//****************            Author: Livio Bisogni             ***************
//*****************************************************************************

/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
                                  INSTRUCTIONS

    Please read the attached `README.md` file.
_____________________________________________________________________________*/


#include "tank.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "init.h"
#include "easy_pthread_task.h"
#include "easy_pthread_time.h"
#include "user.h"


static double dt;  // integration interval                                  [s]


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INIT_INTEGRATION_INTERVAL:  Initialize the integration interval (i.e., dt)
_____________________________________________________________________________*/

void init_integration_interval() { dt = (double)TSCALE * T_TANKS / 1000; }
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    STORE_PHI_IN:   Store the current value of phi_in0 for the i-th tank in its
                    (circular) buffer
_____________________________________________________________________________*/

void store_phi_in(int i)
{
    int k;  // top element index

    assert(i >= 0 && i < NUM_TANKS);

    pthread_mutex_lock(&mux[i]);
    k                            = tank[i].phi.phi_in_top;
    k                            = (k + 1) % PHI_IN_MAX_BUF_LEN;
    tank[i].phi.phi_in_buffer[k] = tank[i].phi.in;
    tank[i].phi.phi_in_top       = k;
    pthread_mutex_unlock(&mux[i]);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    RETRIEVE_PHI_IN:    Retrieve m-th past value of phi_in0 for the i-th tank
                        (from its circular buffer)
_____________________________________________________________________________*/

double retrieve_phi_in(int m, int i)
{
    int    k;  // trail index
    double phi_in0;
    int    phi_in_top;

    pthread_mutex_lock(&mux[i]);
    phi_in_top = tank[i].phi.phi_in_top;
    k          = (phi_in_top + PHI_IN_MAX_BUF_LEN - m) % PHI_IN_MAX_BUF_LEN;
    phi_in0    = tank[i].phi.phi_in_buffer[k];
    pthread_mutex_unlock(&mux[i]);

    return phi_in0;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    TANK_AREA:  Compute tank cross-sectional area (which is a disc of radius r)
_____________________________________________________________________________*/

double tank_area(double r)
{
    return PI * pow(r, 2);  // [m]
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    MA_FILTER:  The moving average (MA) filter is used for smoothing the
                control action phi_in0 of the i-th tank; w is the window size
                (aka filter length)
_____________________________________________________________________________*/

double ma_filter(double current_phi_in, int i)
{
    assert(w <= PHI_IN_MAX_BUF_LEN);

    int    w_local;
    int    k;                // k-th past index;        k = 0, ..., w - 1
    int    samples;          // sample counter;         samples = 1, ..., w
    double phi_in_k;         // k-th value of phi_in0
    double filtered_phi_in;  // filter output

    pthread_mutex_lock(&mux_w);
    w_local = w;
    pthread_mutex_unlock(&mux_w);

    samples         = 0;
    filtered_phi_in = 0;

    // 1-st sample
    samples++;
    phi_in_k        = current_phi_in;
    filtered_phi_in = filtered_phi_in + (phi_in_k - filtered_phi_in) / samples;

    // 2-nd, ..., w-th sample       /* Note that k starts from 0 (which */
    for (k = 0; k < w_local; k++) { /* is "already" the past), not from 1 */
        samples++;
        phi_in_k = retrieve_phi_in(k, i);
        // Average phi_in_k (in an incremental way)
        filtered_phi_in =
            filtered_phi_in + (phi_in_k - filtered_phi_in) / samples;
    }

    return filtered_phi_in;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    SATURATE:   Constrain value to stay in [value_min, value_max]
_____________________________________________________________________________*/

double saturate(double value, double value_min, double value_max)
{
    if (value > value_max)
        value = value_max;  // ..., value_max]
    else if (value < value_min)
        value = value_min;  // [value_min, ...

    return value;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    CHECK_TOLERANCES:   Check whether currently proposed control action u (aka
                        phi_in0) is varying too little, that is if at least
                        one of the three conditions on:
                        * phi_e0_r = rounded(|phi_e0|) = rounded(|u - phi_in1|)
                        (and/or on)
                        * h_e0_r = rounded(|h_e0|) = rounded(|h_des0 - h_meas0|)
                        is met.
                        The three conditional expressions are:
                        1) (phi_e0_r < TOL_1_PHI) && (h_e0_r < TOL_1_H)
                        2) (phi_e0_r < TOL_2_PHI)
                        3) (h_e0_r < TOL_2_H)
                        Function used for avoiding negligible changes of
                        control action.
_____________________________________________________________________________*/

int check_tolerances(double u, double phi_in1, double h_e0)
{
    double phi_e0_r; /* |phi_in(0) - phi_in(-1)| (rounded to PHI_DIGITS     [%]
                        decimal places) */
    double h_e0_r;   /* |h_e0| = |h_des(0) - h_meas(0)| (rounded to         [%]
                        PHI_DIGITS - 1 decimal places) */
    // Four tolerances to avoid negligible changes of phi_in0:
    double phi_e0_r_tol1;  // first tolerance on phi_e0_r; for condition 1a [%]
    double h_e0_r_tol1;    // first tolerance on h_e0_r; for condition 1b   [m]
    double phi_e0_r_tol2;  // second tolerance on phi_e0_r; for condition 2 [%]
    double h_e0_r_tol2;    // second tolerance on h_e0_r; for condition 3   [m]
    // Three conditional expressions to avoid negligible changes of phi_in0:
    int cond1;  // first condition on phi_e0_r and first condition on h_e0_r
    int cond2;  // second condition on phi_e0_r
    int cond3;  // second condition on h_e0_r

    // Compute error |u - phi_in1|, rounded to PHI_DIGITS decimal places
    phi_e0_r = round_first_decimals(fabs(u - phi_in1), PHI_DIGITS);
    /* Compute error |h_e0| = |h_des0 - h_meas0|, rounded to PHI_DIGITS - 1
       decimal places */
    h_e0_r = round_first_decimals(fabs(h_e0), PHI_DIGITS - 1);

    // Four tolerances ...
    phi_e0_r_tol1 = TOL_1_PHI;
    h_e0_r_tol1   = TOL_1_H;
    phi_e0_r_tol2 = TOL_2_PHI;
    h_e0_r_tol2   = TOL_2_H;
    // ... and three conditions
    cond1 = (phi_e0_r < phi_e0_r_tol1) && (h_e0_r < h_e0_r_tol1);
    cond2 = (phi_e0_r < phi_e0_r_tol2);
    cond3 = (h_e0_r < h_e0_r_tol2);

    if (cond1 || cond2 || cond3)
        return 1;
    else
        return 0;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    UPDATE_TANK:    For the i-th tank control its inlet flow valve openness
                    phi_in0 (within [0, 1]) and compute its real water level h0
_____________________________________________________________________________*/

void update_tank(int i)
{
    // Various water levels:
    double h_des0;   // (current) desired water level                       [m]
    double h_meas0;  // current (k = 0) water level measurement reading     [m]
    double h0;       // current (k = 0) real water level                    [m]
    double h1;       // last (k = -1) real water level                      [m]
    // Controller:
    double kp;         // proportional gain                                 []
    double ki;         // integral gain                                     [Hz]
    double kd;         // derivative gain                                   [s]
    double f;          // feedforward dry (0) / wet (1); in [0, 1]          [%]
    double pAction;    // proportional action                               []
    double iAction;    // integral action                                   []
    double dAction;    // derivative action                                 []
    double fAction;    // feedforward action                                []
    double delta_pid;  // pid actions                                       []
    double u;          // control variable (it's gonna be phi_in0)          []
    // Valves (inlet and outlet):
    double phi_in0;   // current (k = 0) inlet valve position; in [0, 1]    []
    double phi_in1;   // last (k = -1) inlet valve position; in [0, 1]      []
    double phi_out0;  // current (k = 0) outlet valve position; in [0, 1]    []
    // Tank parameters:
    double k_local;   // outlet valve constant, aka flow coefficient    [m^2/s]
    double r;         // tank radius                                        [m]
    double A;         // tank cross-sectional area                      [m^2]
    double height;    // tank height                                        [m]
    double q_in_max;  // maximum volumetric inflow rate                 [m^3/s]
    // Control errors:
    double h_e0;  // h_des(0) - h_meas(0)                                   [m]
    double h_e1;  // h_des(-1) - h_meas(-1)                                 [m]
    double h_e2;  // h_des(-2) - h_meas(-2)                                 [m]

    assert(i < NUM_TANKS);
    assert(i >= 0);

    // Retrieve all the values for the i-th tank:
    pthread_mutex_lock(&mux[i]);
    h_des0   = tank[i].lev.h_des;
    h_meas0  = tank[i].lev.h_meas0;
    phi_out0 = tank[i].phi.out;
    phi_in0  = tank[i].phi.in;
    h0       = tank[i].lev.h;
    h_e1     = tank[i].lev.h_e0;
    h_e2     = tank[i].lev.h_e1;
    pthread_mutex_unlock(&mux[i]);
    pthread_mutex_lock(&mux_pid);
    kp = pid.kp;
    ki = pid.ki;
    kd = pid.kd;
    f  = pid.f;
    pthread_mutex_unlock(&mux_pid);
    pthread_mutex_lock(&mux_dim);
    r      = dim.r;
    height = dim.height;
    pthread_mutex_unlock(&mux_dim);
    pthread_mutex_lock(&mux_k);
    k_local = k;
    pthread_mutex_unlock(&mux_k);

    phi_in1  = phi_in0;
    A        = tank_area(r);
    q_in_max = (double)K_IN_DEFAULT * height;  // Equation 2

    // Compute current (k = 0) water height error
    h_e0 = h_des0 - h_meas0;  // Equation 7

    // Compute controller actions:
    pAction = kp * (h_e0 - h_e1) / height;                  // Equation 8
    iAction = ki * dt * h_e0 / height;                      // Equation 9
    dAction = kd / dt * (h_e0 - 2 * h_e1 + h_e2) / height;  // Equation 10
    fAction = phi_out0 * k_local * h_des0 / q_in_max;       // Equation 13

    delta_pid = pAction + iAction + dAction;  // Equation 11

    // Compute controller output
    u = (1 - f) * (phi_in1 + delta_pid) + f * fAction;  // Equation 14

    // Quantize controller output (to PHI_DIGITS decimal places)
    u = round_first_decimals(u, PHI_DIGITS);

    // Check for avoiding negligible changes of phi_in0
    if (check_tolerances(u, phi_in1, h_e0))
        u = phi_in1;
    else
        /* Filter the i-th controller output value with a moving average (MA)
           filter */
        u = ma_filter(u, i);  // Equation 15

    // Saturation: constrain the inlet valve position to stay in [0, 1]
    phi_in0 = saturate(u, 0, 1);  // Equation 16

    // Quantize controller output again (command to be sent to actuator)
    phi_in0 = round_first_decimals(phi_in0, PHI_DIGITS);

    // Update last (h(1)) and compute current (h(0)) tank real water levels
    h1 = h0;
    h0 = (phi_in0 * q_in_max * dt + A * h1) /
         (A + phi_out0 * k_local * dt);  // Equation 6

    // Handle potential water overflows and "underflows"
    h0 = saturate(h0, 0, height);  // Equation 17

    pthread_mutex_lock(&mux[i]);
    tank[i].phi.in   = phi_in0;  // Save command for actuating the inlet valve
    tank[i].lev.h    = h0;       // Save current (k = 0) real level of water
    tank[i].lev.h_e0 = h_e0;     // Save current (k = 0) error
    tank[i].lev.h_e1 = h_e1;     // Save previous (k = -1) error
    pthread_mutex_unlock(&mux[i]);
    store_phi_in(i);  // Store actuator action in the circular buffer
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    TANK_TASK:  Body of i-th tank task
_____________________________________________________________________________*/

void *tank_task(void *arg)
{
    int i;                     // tank index
    int j;                     /* task index (i-th tank task);
                                  it's whortwhile to mention that for
                                  a tank task i and j are equal */
    struct timespec t1;        // t1:   time before task execution
    struct timespec t2;        // t2:   time after task execution
    double          rt_value;  // response_time[j] = t2 - t1                [ms]
    unsigned long   m;         // task execution counter
    char            s[50];     // for printing task name on the terminal

    m = 0;
    j = task_get_index(arg);
    i = j;
    task_set_activation(j);

    /* Continuous update of i-th tank real water level and control of its inlet
       valve position (within 0% - totally closed - and 100% - totally open) */
    while (!end) {
        clock_gettime(CLOCK_MONOTONIC, &t1);  // t1

        update_tank(i);

        clock_gettime(CLOCK_MONOTONIC, &t2);       // t2
        rt_value = get_time_diff_in_ms(&t2, &t1);  // t2 - t1
        manage_rt(j, m, rt_value);
        m++;

        task_check_deadline_miss(j);
        task_wait_for_period(j);
    }

    if (WOET == ON) {  // Print (some of the) response times on terminal
        sprintf(s, "************   Tank[%i] Task   ************\n", i + 1);
        print_dm_rt(j, s);
    }

    return 0;
}
//-----------------------------------------------------------------------------