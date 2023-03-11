//*****************************************************************************
//********** EASY_PTHREAD_TIME.H - Header file of easy_pthread_time.c *********
//**********                  Author: Livio Bisogni                   *********
//*****************************************************************************

#ifndef EASY_PTHREAD_TIME_H
#define EASY_PTHREAD_TIME_H

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


#endif  // EASY_PTHREAD_TIME_H