//*****************************************************************************
//******************** MYPTIME.H - Header file of myptime.c *******************
//********************        Author: Livio Bisogni         *******************
//********************    © 2021 REAL-TIME INDUSTRY Inc.    *******************
//*****************************************************************************

#ifndef MYPTIME_H
#define MYPTIME_H

#include <time.h>


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
	FUNCTION PROTOTYPES
_____________________________________________________________________________*/

void time_copy(struct timespec *td, struct timespec ts);

void time_add_ms(struct timespec *t, int ms);

int time_cmp(struct timespec t1, struct timespec t2);

double timespec2ms(struct timespec t);

double get_time_diff_in_ms(struct timespec *t1, struct timespec *t2);

//-----------------------------------------------------------------------------


#endif	// MYPTIME_H