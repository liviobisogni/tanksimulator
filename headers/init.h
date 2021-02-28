//*****************************************************************************
//*********************** INIT.H - Header file of init.c **********************
//***********************     Author: Livio Bisogni      **********************
//*********************** © 2021 REAL-TIME INDUSTRY Inc. **********************
//*****************************************************************************

/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
                                  INSTRUCTIONS

    Please read the attached `README.md` file.
_____________________________________________________________________________*/


#ifndef INIT_H
#define INIT_H

// #define NDEBUG	// If uncommented, it disables error checks
#define _GNU_SOURCE

#include <allegro.h>
#include <assert.h>
#include "myptask.h"


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    GLOBAL CONSTANTS
_____________________________________________________________________________*/
#define NUM_TANKS 5  // number of tanks

//-----------------------------------------------------------------------------
/* Graphics */
#define BITS                      32  // number of bits to be used for color representation
#define RES_X                     1280  // game resolution X					[pixel]
#define RES_Y                     720   // game resolution Y					[pixel]
#define M2P_SCALE                 50  // 1 [m] = M2P_SCALE [pixel] (scale factor)
#define TANKSIM_X                 30         //									[pixel]
#define TANKSIM_Y                 TANKSIM_X  // 								[pixel]
#define TANKSIM_WIDTH             880        //									[pixel]
#define TANKSIM_HEIGHT            660        //									[pixel]
#define COMMANDS_X                940        //									[pixel]
#define COMMANDS_Y                TANKSIM_Y  // 								[pixel]
#define COMMANDS_ROWS             11         //									[pixel]
#define COMMANDS_WIDTH            320        //									[pixel]
#define COMMANDS_HEIGHT           COMMANDS_ROWS *ROW_OFFSET + WINDOW_OFFSET  //	[pixel]
#define PARAMETERS_X              940  //										[pixel]
#define PARAMETERS_Y              COMMANDS_Y + COMMANDS_HEIGHT + TANKSIM_Y  //	[pixel]
#define PARAMETERS_WIDTH          COMMANDS_WIDTH  //							[pixel]
#define PARAMETERS_HEIGHT         NUM_PARAMS *ROW_OFFSET + WINDOW_OFFSET  // 	[pixel]
#define DM_X                      940  //										[pixel]
#define DM_Y                      PARAMETERS_Y + PARAMETERS_HEIGHT + TANKSIM_Y  //[pixel]
#define DM_WIDTH                  COMMANDS_WIDTH  //							[pixel]
#define DM_HEIGHT                 (NUM_TASKS) * ROW_OFFSET + 2 * WINDOW_OFFSET + 2  //[pixel]
#define COL1_OFFSET               15   //										[pixel]
#define COL2_OFFSET               100  //										[pixel]
#define COL3_OFFSET               160  //										[pixel]
#define COL4_OFFSET               270  //										[pixel]
#define ROW_OFFSET                18   //										[pixel]
#define WINDOW_OFFSET             10   //										[pixel]
#define CHAR_SPACING              20   //										[pixel]
#define NUM_PARAMS                8    //										[pixel]
#define RADIUS2POINTER            0.9  // dial pointer to knob radius ratio; in [0,1]
#define RADIUS2TRIANGLE           0.6  // dial pointer to triangle radius ratio; in [0,1]
#define MAX_TANK_BORDER_THICKNESS 5    // tank borders thickness		[pixel]
#define MAX_RADIUS_THICKNESS      6    // radius thickness					[pixel]
#define YCIRCLE_OFFSET            24   // knob y coordinate offset			[pixel]
#define CIRCLE_RADIUS             0.6  // knob radius						[m]
#define POINTER_CIRCLE_RADIUS     2  // radius of the pivot of the knobs	[pixel]
#define H_DES_LINE_WIDTH          5  //										[pixel]
//-----------------------------------------------------------------------------
/* Colors */
#define BLACK      makecol(0, 0, 0)        // background color
#define WHITE      makecol(255, 255, 255)  // text color
#define GRAY       makecol(200, 200, 200)  // knobs circle color
#define LIGHTBLACK makecol(20, 15, 14)     // knobs radius color
#define PINKYRED   makecol(210, 45, 100)   // tab windows color
#define RED        makecol(250, 40, 5)     // sliders (inner) and knobs arc col.
#define DARKGRAY   makecol(80, 80, 80)     // sliders (outer part) color
#define ORANGE     makecol(250, 150, 10)   // zoom percentage text color
#define GREEN      makecol(74, 216, 3)     // text color
#define AZURE      makecol(16, 150, 240)   // water color
//-----------------------------------------------------------------------------
/* Tasks, i.e.:
 *) NUM_TANKS tanks
 *) NUM_TANKS sensors
 *) 1 user
 *) 1 graphics */
#define NUM_TASKS                                                              \
    2 * NUM_TANKS + 2   // Sum of: NUM_TANKS tank tasks, NUM_TANKS sensor tasks,
                        // 1 user task, and 1 graphics task
#define P_TANKS     4   // tanks priority; in {1, ..., LINUX_MAX_PRIO}		[]
#define T_TANKS     40  // tanks period										[ms]
#define DL_TANKS    40  // tanks relative deadline							[ms]
#define P_SENSORS   5   // sensors priority; in {1, ..., LINUX_MAX_PRIO}	[]
#define T_SENSORS   40  // sensors period									[ms]
#define DL_SENSORS  40  // sensors relative deadline						[ms]
#define P_GRAPHICS  2   // graphics priority; in {1, ..., LINUX_MAX_PRIO}	[]
#define T_GRAPHICS  40  // graphics period									[ms]
#define DL_GRAPHICS 40  // graphics relative deadline						[ms]
#define P_USER      3   // user priority; in {1, ..., LINUX_MAX_PRIO}		[]
#define T_USER      40  // user period										[ms]
#define DL_USER     40  // user relative deadline							[ms]
//-----------------------------------------------------------------------------
/* Tanks */
#define TSCALE             1 / 2  // time scale factor
#define PERCENT            100
#define PI                 3.14159
#define KP_MIN             0     // minimum value for proportional gain kp		[]
#define KP_MAX             20    // maximum value for proportional gain kp		[]
#define KP_STEPSIZE        0.2   // stepsize value for proportional gain kp		[]
#define KP_DEFAULT         1     // default value for proportional gain kp		[]
#define KP_DIGITS          1     // decimal places for proportional gain kp		[]
#define KI_MIN             0     // minimum value for integral gain ki			[]
#define KI_MAX             200   // maximum value for integral gain ki			[]
#define KI_STEPSIZE        2     // stepsize value for integral gain ki			[]
#define KI_DEFAULT         30    // default value for integral gain ki			[]
#define KI_DIGITS          0     // decimal places for integral gain kp			[]
#define KD_MIN             0     // minimum value for derivative gain kd		[]
#define KD_MAX             2     // maximum value for derivative gain kd		[]
#define KD_STEPSIZE        0.02  // stepsize value for derivative gain kd		[]
#define KD_DEFAULT         0     // default value for derivative gain kd		[]
#define KD_DIGITS          2     // decimal places for derivative gain kd		[]
#define F_MIN              0     // minimum value for feedforward dry/wet		[%]
#define F_MAX              1     // maximum value for feedforward dry/wet		[%]
#define F_STEPSIZE         0.05  // stepsize value for feedforward dry/wet		[%]
#define F_DEFAULT          0.25  // default value for feedforward dry/wet		[%]
#define K_MIN              0.1  // minimum value for outlet valve constant k	[m^2/s]
#define K_MAX              1  // maximum value for outlet valve constant k		[m^2/s]
#define K_STEPSIZE         0.1  // stepsize value for outlet valve constant k	[m^2/s]
#define K_DEFAULT          1  // default value for outlet valve constant k		[m^2/s]
#define K_IN_DEFAULT       1  // default value for inlet valve constant k_in [m^2/s]
#define HEIGHT_MIN         0.5  // minimum value for tank height				[m]
#define HEIGHT_MAX         9    // maximum value for tank height				[m]
#define HEIGHT_STEPSIZE    1.0 / M2P_SCALE  // stepsize	value for tank height[m]
#define HEIGHT_DEFAULT     9    // default value for tank height				[m]
#define R_MIN              0.1  // minimum value for radius r					[m]
#define R_MAX              1    // maximum value for radius r					[m]
#define R_STEPSIZE         0.1  // stepsize value for radius r					[m]
#define R_DEFAULT          0.5  // default value for radius r					[m]
#define H_DEFAULT          2.1  // default value for actual water level hh		[m]
#define H_EST_DEFAULT      2.2  // default value for measured water level h		[m]
#define H_DES_DEFAULT      3    // default value for desired water level h		[m]
#define W_MIN              0    // minimum value for filter window size w		[]
#define W_MAX              20   // maximum value for filter window size w		[]
#define W_STEPSIZE         1    // stepsize value for filter window size w		[]
#define W_DEFAULT          1    // default value for filter window size w		[]
#define U_MIN              0.5  // minimum value for utilization scale factor	[]
#define U_MAX              20   // maximum value for utilization scale factor	[]
#define U_STEPSIZE         1    // stepsize value for utilization scale factor[]
#define U_DEFAULT          10   // default value for utilization scale factor	[]
#define PHI_IN_MAX_BUF_LEN W_MAX  // circular buffer max possible length	[]
#define PHI_IN_DEFAULT     0      // default value for phi_in					[%]
#define PHI_OUT_DEFAULT    0.5    // default value for phi_out					[%]
#define PHI_DIGITS         3      // decimal places for phi_in and phi_out
#define TOL_1_PHI                                                              \
    3.0 * pow(10, -PHI_DIGITS)  // 1-st tolerance on |phi_in(0) - phi_in(-1)|[%]
#define TOL_1_H                                                                \
    2.0 / M2P_SCALE  // 1-st tolerance on |h(0) - h(-1)|					[m]
#define TOL_2_PHI                                                              \
    1.0 * pow(10, -PHI_DIGITS)  // 2-nd tolerance on |phi_in(0) - phi_in(-1)|[%]
#define TOL_2_H                                                                \
    1.0 / M2P_SCALE  // 2-nd tolerance on |h(0) - h(-1)|					[m]
//-----------------------------------------------------------------------------
/* Sensors */
#define SENSOR_MIN  0  // minimum sensor distance							[m]
#define SENSOR_STEP 1  // sensor resolution								[pixel]
//-----------------------------------------------------------------------------
/* User */
#define ON  1  // when (random.) button is pressed or when rt should be printed
#define OFF 0  // when (randomizer) button is released
//-----------------------------------------------------------------------------
/* Printing (on terminal) */
#define WOET         ON  // if ON response times are printed, if OFF they ain't
#define WOET_N_WORST 20  // top n worst response times to be printed
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    STRUCTURES DEFINITION
_____________________________________________________________________________*/

//-----------------------------------------------------------------------------
/* Tank position */
typedef struct
{
    int x;  // x graphics coordinate									[pixel]
    int y;  // y graphics coordinate									[pixel]
} position;
//-----------------------------------------------------------------------------
/* Tank dimension */
typedef struct
{
    double height;  // tank height											[m]
    double r;       // tank radius											[m]
} dimension;
//-----------------------------------------------------------------------------
/* Tank water levels (desired, measured, and actual) */
typedef struct
{
    double h_des;      // desired water level in the tank at time k = 0		[m]
    double h_meas0;    // estimated water level in the tank at time k = 0	[m]
    double h;          // actual (real) water level in the tank at k = 0	[m]
    double h_e0, h_e1; /* differences between desired and estimated water	[m]
                          level at times k = 0 and k = -1, respectively */
} level;
//-----------------------------------------------------------------------------
/* Controller parameters */
typedef struct
{
    double kp;  // proportional gain										[]
    double ki;  // integral gain 											[Hz]
    double kd;  // derivative gain											[s]
    double f;   // feedforward dry (0) / wet (1)							[%]
} pid_controller;
//-----------------------------------------------------------------------------
/* Inlet / outlet spool positions (inlet and outlet valves position values) */
typedef struct
{
    double in;   // current (k=0) inlet valve spool position; value in [0, 1] []
    double out;  // current (k=0) outlet valve spool position; value in [0, 1][]
    double phi_in_buffer[PHI_IN_MAX_BUF_LEN]; /* array of phi_in (circular
                                                 buffer structure) */
    int phi_in_top; /* index of the current element of the circular buffer
                       structure; value in {0, ..., PHI_IN_MAX_BUF_LEN - 1} */
} spool_position;
//-----------------------------------------------------------------------------
/* Tank status */
typedef struct
{
    position       pos;    // graphics coordinates
    level          lev;    // water levels
    spool_position phi;    // adjustable inlet and outlet valves
    int            color;  // tank borders color
} tank_attr;
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    GLOBAL VARIABLES (declaration)
_____________________________________________________________________________*/

/* Simulator variables */
extern int            end;  // end flag (if == 1, the simulator exits)
extern tank_attr      tank[];
extern pid_controller pid;
extern dimension      dim;
extern double         k;  // outlet valve constant							[m^2/s]
extern int            w;  // window size, aka (moving average) filter length	[]
extern double         util_scale;  // utilization scale factor (zoom in/out)	[]
//-----------------------------------------------------------------------------
/* Mutexes */
extern pthread_mutex_t mux[];      // mutexes for tank statuses		(tank)
extern pthread_mutex_t mux_pid;    // mutex for pid controller		(pid)
extern pthread_mutex_t mux_dim;    // mutex for tanks dimension		(dim)
extern pthread_mutex_t mux_k;      // mutex for valve constant		(k)
extern pthread_mutex_t mux_w;      // mutex for filter window size	(w)
extern pthread_mutex_t mux_util;   // mutex for util. scale factor	(util_scale)
extern pthread_mutex_t mux_print;  // mutex for printing
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    FUNCTION PROTOTYPES
_____________________________________________________________________________*/

void manage_rt(int j, unsigned long m, double rt_value);

void print_dm_rt(int j, char s[]);

void init();

//-----------------------------------------------------------------------------


#endif  // INIT_H