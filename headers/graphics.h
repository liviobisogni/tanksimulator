//*****************************************************************************
//****************** GRAPHICS.H - Header file of graphics.c *******************
//******************          Author: Livio Bisogni         *******************
//******************      © 2021 REAL-TIME INDUSTRY Inc.    *******************
//*****************************************************************************

/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
                                  INSTRUCTIONS

    Please read the attached `README.md` file.
_____________________________________________________________________________*/


#ifndef GRAPHICS_H
#define GRAPHICS_H


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    FUNCTION PROTOTYPES
_____________________________________________________________________________*/

double p2m(int value_p);

int m2p(double value_m);

double quantize(double value_m);

void reset_util_inst_max_process();

void draw_randomizer_ellipse(int toggle);

void draw_static_graphics();

void init_graphics_buffers();

void *graphics_task(void *arg);

//-----------------------------------------------------------------------------


#endif  // GRAPHICS_H