//*****************************************************************************
//************** GRAPHICS.C - Handle the whole simulator graphics *************
//**************              Author: Livio Bisogni               *************
//**************          © 2021 REAL-TIME INDUSTRY Inc.          *************
//*****************************************************************************

/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
                                  INSTRUCTIONS

    Please read the attached `README.md` file.
_____________________________________________________________________________*/


#include "graphics.h"
#include <allegro.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include "init.h"
#include "myptask.h"
#include "myptime.h"
#include "user.h"


static BITMAP *static_buffer;        /* buffer for [static] graphics; computed
                                        once and for all (at graphics task
                                        startup) */
static BITMAP *animated_buffer;      /* buffer for [animated] graphics; updated
                                        repeatedly (at each screen refresh) */
static double util_inst_max_process; /* maximum instantaneous utilization of
                                        the process */


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    P2M:    Convert value_p from pixels to meters
_____________________________________________________________________________*/

double p2m(int value_p)
{
    double value_m;  // value expressed in [m]
    value_m = (double)value_p / M2P_SCALE;
    return value_m;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    M2P:    Convert value_m from meters to pixels
_____________________________________________________________________________*/

int m2p(double value_m)
{
    int value_p;  // value expressed in [pixel]
    value_p = round(value_m * M2P_SCALE);
    return value_p;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    QUANTIZE:   Quantize a length (value_m, expressed in meters)
_____________________________________________________________________________*/

double quantize(double value_m)
{
    double value_m_quantized;  //  quantized length                         [m]
    value_m_quantized = p2m(m2p(value_m));
    return value_m_quantized;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    RESET_UTIL_INST_MAX_PROCESS:    Set util_inst_max_process to 0
_____________________________________________________________________________*/

void reset_util_inst_max_process() { util_inst_max_process = 0; }
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    WRITE_H_MEAS_SLIDER_VALUE:  Write measured water level h_meas0 (left of the
                                tank, in azure) for a tank in position (x, y),
                                a certain height and radius r
                                Graphics: [animated]
_____________________________________________________________________________*/

void write_h_meas_slider_value(int x, int y, double height, double r,
                               double h_meas0)
{
    char s[20];  // string of the numeric value to be written
    int  r_p;    // tank radius                                         [pixel]

    r_p = m2p(r);

    if (h_meas0 < 0)
        h_meas0 = 0;

    // Write the new value (h_meas0)
    sprintf(s, "%5.2f", h_meas0);
    textout_centre_ex(animated_buffer, font, s,
                      x - r_p - 1 * CHAR_SPACING - WINDOW_OFFSET + 6,
                      y - m2p(h_meas0) - ROW_OFFSET / 2 + 3, AZURE, -1);
    sprintf(s, "[m]");
    textout_centre_ex(animated_buffer, font, s,
                      x - r_p - 1 * CHAR_SPACING - WINDOW_OFFSET + 10,
                      y - m2p(h_meas0) + 0 * ROW_OFFSET / 2 + 3, AZURE, -1);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    WRITE_H_DES_SLIDER_VALUE:   Write desired water level h_des (right of the
                                tank, in red); the tank has coordinates (x, y),
                                a certain height and radius r
                                Graphics: [animated]
_____________________________________________________________________________*/

void write_h_des_slider_value(int x, int y, double height, double r,
                              double h_des)
{
    char s[20];  // string containing the numeric value to be written
    int  r_p;    // tank radius                                         [pixel]

    r_p = m2p(r);

    // Write new value (h_des)
    sprintf(s, "%5.2f", h_des);
    textout_centre_ex(animated_buffer, font, s, x + r_p + 1 * CHAR_SPACING - 1,
                      y - m2p(h_des) - ROW_OFFSET / 2 + 3, RED, -1);
    sprintf(s, "[m]");
    textout_centre_ex(animated_buffer, font, s, x + r_p + 1 * CHAR_SPACING + 3,
                      y - m2p(h_des) + 0 * ROW_OFFSET / 2 + 3, RED, -1);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_WATER: Fill tank with water (to real water level h); the tank has
                coordinates (x, y), a certain height and radius r
                Graphics: [animated]
_____________________________________________________________________________*/

void draw_water(int x, int y, double height, double r, double h)
{
    int r_p;  // tank radius                                            [pixel]

    r_p = m2p(r);

    // If there is water in the tank ...
    if (h > p2m(1))
        // ... draw water (in azure)
        rectfill(animated_buffer, x - r_p, y, x + r_p, y - m2p(h), AZURE);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_H_DES_LINE:    Draw a gray-red fader at position (x, y - height) of
                        width 2r, denoting the desired water level h_des
                        Graphics: [animated]
_____________________________________________________________________________*/

void draw_h_des_line(int x, int y, double height, double r, double h_des)
{
    int r_p;  // tank radius                                            [pixel]

    r_p = m2p(r);

    // Draw fader (outer) rectangle (in gray)
    rectfill(animated_buffer, x - r_p + H_DES_LINE_WIDTH - 2,
             y - m2p(h_des) - 2, x + r_p - H_DES_LINE_WIDTH + 2,
             y - m2p(h_des) + 2, DARKGRAY);
    // Draw fader (inner) line (in red)
    line(animated_buffer, x - r_p + H_DES_LINE_WIDTH, y - m2p(h_des),
         x + r_p - H_DES_LINE_WIDTH, y - m2p(h_des), RED);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_CIRCLE:    Draw a gray circle for a knob in position (x_circle,
                    y_circle) and with radius r_circle
                    Graphics: [animated]
_____________________________________________________________________________*/

void draw_circle(int x_circle, int y_circle, int r_circle)
{
    circlefill(animated_buffer, x_circle, y_circle, r_circle, GRAY);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_INNER_CIRCLE:  Draw a small black circle for a knob in position
                        (x_circle, y_circle), representing the center of its
                        pointer
                        Graphics: [animated]
_____________________________________________________________________________*/

void draw_inner_circle(int x_circle, int y_circle)
{
    circlefill(animated_buffer, x_circle, y_circle, POINTER_CIRCLE_RADIUS,
               BLACK);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_RADIUS_LINE:   Draw the black pointer of the knob in position
                        (x_circle, y_circle) and with radius r_circle; it's
                        made up of a long needle and an ending arrow
                        Graphics: [animated]
_____________________________________________________________________________*/

void draw_radius_line(int x_circle, int y_circle, int r_circle, double phi)
{
    int    x_start;      // line starting point x coordinate            [pixel]
    int    y_start;      // line starting point y coordinate            [pixel]
    int    x_end;        // line ending point x coordinate              [pixel]
    int    y_end;        // line ending point y coordinate              [pixel]
    int    x_bl;         // triangle bottom left point x coordinate     [pixel]
    int    y_bl;         // triangle bottom left point y coordinate     [pixel]
    int    x_br;         // triangle bottom right point x coordinate    [pixel]
    int    y_br;         // triangle bottom right point y coordinate    [pixel]
    int    x_t;          // triangle top point x coordinate             [pixel]
    int    y_t;          // triangle top point x coordinate             [pixel]
    double angle_shift;  // triangle base angular shift                 [rad]

    // Compute line (the long needle) coordinates
    x_start = x_circle;
    y_start = y_circle;
    x_end   = x_circle + (RADIUS2POINTER * (double)r_circle) *
                           (double)cos((double)phi * 2 * PI + PI / 2);
    y_end = y_circle + (RADIUS2POINTER * (double)r_circle) *
                           (double)sin((double)phi * 2 * PI + PI / 2);

    // Compute triangle (the ending arrow) coordinates
    angle_shift = 0.2;
    x_bl        = x_circle - 0 +
           (RADIUS2TRIANGLE * (double)r_circle) *
               (double)cos((double)phi * 2 * PI + PI / 2 - angle_shift);
    y_bl = y_circle - 0 +
           (RADIUS2TRIANGLE * (double)r_circle) *
               (double)sin((double)phi * 2 * PI + PI / 2 - angle_shift);
    x_br = x_circle + 0 +
           (RADIUS2TRIANGLE * (double)r_circle) *
               (double)cos((double)phi * 2 * PI + PI / 2 + angle_shift);
    y_br = y_circle + 0 +
           (RADIUS2TRIANGLE * (double)r_circle) *
               (double)sin((double)phi * 2 * PI + PI / 2 + angle_shift);
    x_t = x_end;
    y_t = y_end;

    // Draw pointer needle (a black line)
    line(animated_buffer, x_start, y_start, x_end, y_end, LIGHTBLACK);
    // Draw pointer arrow (a black triangle)
    triangle(animated_buffer, x_bl, y_bl, x_br, y_br, x_t, y_t, LIGHTBLACK);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_KNOB_ARC:  Draw a red arc (from 0 to the current percentage of phi_in)
                    for a knob in position (x_circle, y_circle) and with radius
                    r_circle
                    It displays its corresponding inlet valve current opening[%]
                    Graphics: [animated]
_____________________________________________________________________________*/

void draw_knob_arc(int x_circle, int y_circle, int r_circle, double phi_in)
{
    int t;  // radius thickness index                                   [pixel]
    int phase, ang1, ang2;
    int arc_thickness;

    if (phi_in == 1)
        phi_in = 0.9999;
    phase = -64;
    ang1  = -phi_in * 256 + phase;
    ang2  = phase;

    ang1 = itofix(ang1);
    ang2 = itofix(ang2);

    arc_thickness = 1 + round(r_circle / 10);

    // Draw a thick red arc
    for (t = 1; t <= arc_thickness; t++)
        arc(animated_buffer, x_circle, y_circle, ang1, ang2, r_circle - t, RED);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    WRITE_KNOB_VALUE:   Write a value (phi) upon a knob in position (x_circle,
                        y_circle) and with radius r_circle, denoting the valve
                        openness (expressed as percentage)
                        Graphics: [animated]
_____________________________________________________________________________*/

void write_knob_value(int x_circle, int y_circle, int r_circle, double phi)
{
    char s[20];  // string containing the numeric value to be written

    // Round to PHI_DIGITS significant figures
    if (PHI_DIGITS == 1)
        sprintf(s, "%i %%", (int)round_first_decimals(phi * PERCENT, 1));
    else if (PHI_DIGITS == 2)
        sprintf(s, "%i %%", (int)round_first_decimals(phi * PERCENT, 2));
    else if (PHI_DIGITS == 3)
        sprintf(s, "%5.1f %%", round_first_decimals(phi * PERCENT, 3));
    else if (PHI_DIGITS == 4)
        sprintf(s, "%6.2f %%", round_first_decimals(phi * PERCENT, 4));

    // Write valve openness value
    textout_centre_ex(animated_buffer, font, s, x_circle,
                      y_circle - (r_circle + YCIRCLE_OFFSET / 2), WHITE, -1);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_IN_KNOB:   Draw the inlet knob of the tank at position (x, y) and with
                    a certain height
                    Graphics: [animated]
_____________________________________________________________________________*/

void draw_in_knob(int x, int y, double height)
{
    int r_circle, x_circle, y_circle;  // inlet knob circle parameters [pixel]

    r_circle = m2p(CIRCLE_RADIUS);
    x_circle = x;
    y_circle = y - m2p(height) - MAX_TANK_BORDER_THICKNESS - r_circle -
               YCIRCLE_OFFSET / 2;

    draw_circle(x_circle, y_circle, r_circle);
    draw_inner_circle(x_circle, y_circle);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    UPDATE_IN_KNOB: Update the inlet knob of the tank at position (x, y) and
                    with a certain height:
                        1) move its pointer to phi_in
                        2) draw a red arc till that point
                        3) write phi_in percentage on top of the knob (ranging
                           from 0% to 100%)
                    Graphics: [animated]
_____________________________________________________________________________*/

void update_in_knob(int x, int y, double height, double phi_in)
{
    int r_circle, x_circle, y_circle;  // inlet knob circle parameters  [pixel]

    r_circle = m2p(CIRCLE_RADIUS);
    x_circle = x;
    y_circle = y - m2p(height) - MAX_TANK_BORDER_THICKNESS - r_circle -
               YCIRCLE_OFFSET / 2;

    draw_knob_arc(x_circle, y_circle, r_circle, phi_in);
    draw_radius_line(x_circle, y_circle, r_circle, phi_in);
    write_knob_value(x_circle, y_circle, r_circle, phi_in);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_OUT_KNOB:  Draw the outlet knob of the tank at position (x, y)
                    Graphics: [animated]
_____________________________________________________________________________*/

void draw_out_knob(int x, int y)
{
    int r_circle, x_circle, y_circle;  // outlet knob circle parameters [pixel]

    r_circle = m2p(CIRCLE_RADIUS);
    x_circle = x;
    y_circle = y + MAX_TANK_BORDER_THICKNESS + r_circle + YCIRCLE_OFFSET;

    draw_circle(x_circle, y_circle, r_circle);
    draw_inner_circle(x_circle, y_circle);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    UPDATE_OUT_KNOB:    Update the outlet knob of the tank at position (x, y):
                            1) move its pointer to phi_out
                            2) draw a red arc till that point
                            3) write phi_out percentage on top of the knob
                               (ranging from 0% to 100%)
                        Graphics: [animated]
_____________________________________________________________________________*/

void update_out_knob(int x, int y, double phi_out)
{
    int r_circle, x_circle, y_circle;  // outlet knob circle parameters [pixel]

    r_circle = m2p(CIRCLE_RADIUS);
    x_circle = x;
    y_circle = y + MAX_TANK_BORDER_THICKNESS + r_circle + YCIRCLE_OFFSET;

    draw_knob_arc(x_circle, y_circle, r_circle, phi_out);
    draw_radius_line(x_circle, y_circle, r_circle, phi_out);
    write_knob_value(x_circle, y_circle, r_circle, phi_out);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_TANK_BORDERS:  Draw a liquid container in a given color; the tank is
                        located at position (x, y) and has a certain height and
                        radius r
                        Graphics: [animated]
_____________________________________________________________________________*/

void draw_tank_borders(int x, int y, double height, double r, int color)
{
    int r_p;       // tank radius                                       [pixel]
    int height_p;  // tank height                                       [pixel]

    r_p      = m2p(r);
    height_p = m2p(height);

    // Draw left border
    rectfill(animated_buffer, x - r_p - MAX_TANK_BORDER_THICKNESS,
             y + MAX_TANK_BORDER_THICKNESS, x - r_p,
             y - height_p + 0 * MAX_TANK_BORDER_THICKNESS - 0, color);

    // Draw right border
    rectfill(animated_buffer, x + r_p + MAX_TANK_BORDER_THICKNESS,
             y + MAX_TANK_BORDER_THICKNESS, x + r_p,
             y - height_p + 0 * MAX_TANK_BORDER_THICKNESS - 0, color);

    // Draw bottom border
    rectfill(animated_buffer, x - r_p - MAX_TANK_BORDER_THICKNESS,
             y + MAX_TANK_BORDER_THICKNESS, x + r_p, y, color);

    // Draw (upper) left triangle
    triangle(animated_buffer, x - r_p - MAX_TANK_BORDER_THICKNESS, y - height_p,
             x - r_p - MAX_TANK_BORDER_THICKNESS,
             y - height_p - MAX_TANK_BORDER_THICKNESS, x - r_p, y - height_p,
             color);

    // Draw (upper) right triangle
    triangle(animated_buffer, x + r_p + MAX_TANK_BORDER_THICKNESS, y - height_p,
             x + r_p + MAX_TANK_BORDER_THICKNESS,
             y - height_p - MAX_TANK_BORDER_THICKNESS, x + r_p, y - height_p,
             color);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_COMMANDS_TAB:  Write actions + commands tab (top-right)
                        Graphics: [static]
_____________________________________________________________________________*/

void draw_commands_tab()
{
    int  l;              // action index
    char s_action[20];   // l-th action string
    char s_command[30];  // l-th command string

    // Draw tab borders (pinkyred color)
    rect(static_buffer, COMMANDS_X, COMMANDS_Y, COMMANDS_X + COMMANDS_WIDTH,
         COMMANDS_Y + COMMANDS_HEIGHT, PINKYRED);

    // Write tab field names
    textout_ex(static_buffer, font, "Commands", COMMANDS_X + COL1_OFFSET / 2,
               COMMANDS_Y - 1.5 * WINDOW_OFFSET, PINKYRED, -1);

    textout_ex(static_buffer, font, "Actions", COMMANDS_X + COL2_OFFSET,
               COMMANDS_Y - 1.5 * WINDOW_OFFSET, PINKYRED, -1);

    // Write actions + commands:
    for (l = 0; l < COMMANDS_ROWS; l++) {
        // 1) kp
        if (l == 0) {
            sprintf(s_command, "p- / P+");
            sprintf(s_action, "vary proportional gain Kp");
        }

        // 2) ki
        if (l == 1) {
            sprintf(s_command, "i- / I+");
            sprintf(s_action, "vary integral gain Ki");
        }

        // 3) kd
        if (l == 2) {
            sprintf(s_command, "d- / D+");
            sprintf(s_action, "vary derivative gain Kd");
        }

        // 4) f
        if (l == 3) {
            sprintf(s_command, "f- / F+");
            sprintf(s_action, "vary feedforward dry/wet f");
        }

        // 5) w
        if (l == 4) {
            sprintf(s_command, "w- / W+");
            sprintf(s_action, "vary filter window size W");
        }

        // 6) k
        if (l == 5) {
            sprintf(s_command, "k- / K+");
            sprintf(s_action, "vary valve constant k");
        }

        // 7) r
        if (l == 6) {
            sprintf(s_command, "r- / R+");
            sprintf(s_action, "vary tanks radius r");
        }

        // 8) h
        if (l == 7) {
            sprintf(s_command, "h- / H+");
            sprintf(s_action, "vary tanks height h");
        }

        // 9) u
        if (l == 8) {
            sprintf(s_command, "u- / U+");
            sprintf(s_action, "vary utilization scale u");
        }

        // 10) SPACE
        if (l == 9) {
            sprintf(s_command, "SPACE");
            sprintf(s_action, "restore default values");
        }

        // 11) ESC
        if (l == 10) {
            sprintf(s_command, "ESC");
            sprintf(s_action, "exit the program");
        }

        textout_ex(static_buffer, font, s_command, COMMANDS_X + COL1_OFFSET,
                   COMMANDS_Y + WINDOW_OFFSET + l * ROW_OFFSET, GREEN, -1);
        textout_ex(static_buffer, font, s_action, COMMANDS_X + COL2_OFFSET,
                   COMMANDS_Y + WINDOW_OFFSET + l * ROW_OFFSET, WHITE, -1);
    }
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_PARAMETERS_TAB:    Write parameters names + ranges + units of
                            measurement tab (middle-right)
                            Graphics: [static]
_____________________________________________________________________________*/

void draw_parameters_tab()
{
    int  p;                // parameter index
    char s_parameter[20];  // p-th parameter string
    char s_range[20];      // p-th parameter range
    char s_unit[20];       // p-th parameter SI unit of measurement

    // Draw tab borders
    rect(static_buffer, PARAMETERS_X, PARAMETERS_Y,
         PARAMETERS_X + PARAMETERS_WIDTH, PARAMETERS_Y + PARAMETERS_HEIGHT,
         PINKYRED);

    // Write tab field names
    textout_ex(static_buffer, font, "Parameters",
               PARAMETERS_X + COL1_OFFSET / 2,
               PARAMETERS_Y - 1.5 * WINDOW_OFFSET, PINKYRED, -1);
    textout_ex(static_buffer, font, "Values", PARAMETERS_X + COL2_OFFSET,
               PARAMETERS_Y - 1.5 * WINDOW_OFFSET, PINKYRED, -1);
    textout_ex(static_buffer, font, "Ranges", PARAMETERS_X + COL3_OFFSET,
               PARAMETERS_Y - 1.5 * WINDOW_OFFSET, PINKYRED, -1);
    textout_ex(static_buffer, font, "Units", PARAMETERS_X + COL4_OFFSET,
               PARAMETERS_Y - 1.5 * WINDOW_OFFSET, PINKYRED, -1);

    // Write various stuff (parameter names + ranges + units of measurement):
    for (p = 0; p < NUM_PARAMS; p++) {
        // 1) kp
        if (p == 0) {
            sprintf(s_parameter, "Kp");
            sprintf(s_range, "[%.1f; %.1f]", (double)KP_MIN, (double)KP_MAX);
            sprintf(s_unit, "-");
        }

        // 2) ki
        if (p == 1) {
            sprintf(s_parameter, "Ki");
            sprintf(s_range, "[%.0f; %.0f]", (double)KI_MIN, (double)KI_MAX);
            sprintf(s_unit, "Hz");
        }

        // 3) kd
        if (p == 2) {
            sprintf(s_parameter, "Kd");
            sprintf(s_range, "[%.2f; %.2f]", (double)KD_MIN, (double)KD_MAX);
            sprintf(s_unit, "s");
        }

        // 4) f
        if (p == 3) {
            sprintf(s_parameter, "f");
            sprintf(s_range, "[%i; %i]", F_MIN * PERCENT, F_MAX * PERCENT);
            sprintf(s_unit, "%%");
        }

        // 5) w
        if (p == 4) {
            sprintf(s_parameter, "W");
            sprintf(s_range, "[%i; %i]", (int)W_MIN, (int)W_MAX);
            sprintf(s_unit, "-");
        }

        // 6) k
        if (p == 5) {
            sprintf(s_parameter, "k");
            sprintf(s_range, "[%.1f; %.1f]", (double)K_MIN, (double)K_MAX);
            sprintf(s_unit, "m²/s");
        }

        // 7) r
        if (p == 6) {
            sprintf(s_parameter, "r");
            sprintf(s_range, "[%.1f; %.1f]", (double)R_MIN, (double)R_MAX);
            sprintf(s_unit, "m");
        }

        // 8) height
        if (p == NUM_PARAMS - 1) {
            sprintf(s_parameter, "h");
            sprintf(s_range, "[%.2f; %.2f]", (double)HEIGHT_MIN,
                    (double)HEIGHT_MAX);
            sprintf(s_unit, "m");
        }

        textout_ex(static_buffer, font, s_parameter, PARAMETERS_X + COL1_OFFSET,
                   PARAMETERS_Y + WINDOW_OFFSET + p * ROW_OFFSET, GREEN, -1);
        textout_ex(static_buffer, font, s_range, PARAMETERS_X + COL3_OFFSET,
                   PARAMETERS_Y + WINDOW_OFFSET + p * ROW_OFFSET, WHITE, -1);
        textout_ex(static_buffer, font, s_unit,
                   PARAMETERS_X + COL4_OFFSET + COL1_OFFSET,
                   PARAMETERS_Y + WINDOW_OFFSET + p * ROW_OFFSET, WHITE, -1);
    }
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    UPDATE_PARAMETERS_TAB:  Write parameters values (middle-right)
                            Graphics: [animated]
_____________________________________________________________________________*/

void update_parameters_tab()
{
    int    p;                                  // parameter index
    char   s_value[20];                        // p-th parameter value
    int    w_local;                            // simulator integer parameter
    double kp, ki, kd, f, k_local, height, r;  // simulator float parameters

    // Write parameter values:
    for (p = 0; p < NUM_PARAMS; p++) {
        // kp
        if (p == 0) {
            pthread_mutex_lock(&mux_pid);
            kp = pid.kp;
            pthread_mutex_unlock(&mux_pid);
            sprintf(s_value, "%.1f", kp);
        }

        // ki
        if (p == 1) {
            pthread_mutex_lock(&mux_pid);
            ki = pid.ki;
            pthread_mutex_unlock(&mux_pid);
            sprintf(s_value, "%.0f", ki);
        }

        // kd
        if (p == 2) {
            pthread_mutex_lock(&mux_pid);
            kd = pid.kd;
            pthread_mutex_unlock(&mux_pid);
            sprintf(s_value, "%.2f", kd);
        }

        // f
        if (p == 3) {
            pthread_mutex_lock(&mux_pid);
            f = pid.f;
            pthread_mutex_unlock(&mux_pid);
            sprintf(s_value, "%i", (int)(round(f * PERCENT)));
        }

        // w
        if (p == 4) {
            pthread_mutex_lock(&mux_w);
            w_local = w;
            pthread_mutex_unlock(&mux_w);
            sprintf(s_value, "%i", w_local);
        }

        // k
        if (p == 5) {
            pthread_mutex_lock(&mux_k);
            k_local = k;
            pthread_mutex_unlock(&mux_k);
            sprintf(s_value, "%.1f", k_local);
        }

        // r
        if (p == 6) {
            pthread_mutex_lock(&mux_dim);
            r = dim.r;
            pthread_mutex_unlock(&mux_dim);
            sprintf(s_value, "%.1f", r);
        }

        // height
        if (p == NUM_PARAMS - 1) {
            pthread_mutex_lock(&mux_dim);
            height = dim.height;
            pthread_mutex_unlock(&mux_dim);
            sprintf(s_value, "%.2f", height);
        }

        textout_ex(animated_buffer, font, s_value, PARAMETERS_X + COL2_OFFSET,
                   PARAMETERS_Y + WINDOW_OFFSET + p * ROW_OFFSET, WHITE, -1);
    }
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_TANKS_TAB: Draw main (tanks) tab and main (colored) title
                    Graphics: [animated]
_____________________________________________________________________________*/

void draw_tanks_tab()
{
    char s[20];       // string containing the letters to be written
    int  length_s;    // character (of the the string s) counter
    int  c;           // character index
    char t[20];       // c-th char of the title string
    int  color;       // character color
    int  color_rand;  // tank borders color

    sprintf(s, "TANKS Simulator");  // Program title

    for (length_s = 0; s[length_s] != '\0'; ++length_s)
        ;

    // Draw tab borders (pinkyred color)
    rect(animated_buffer, TANKSIM_X, TANKSIM_Y, TANKSIM_X + TANKSIM_WIDTH,
         TANKSIM_Y + TANKSIM_HEIGHT, PINKYRED);

    // Write simulator main title (c-th letter is colored the same as i-th tank)
    for (c = 0; c < length_s; c++) {
        if (c < NUM_TANKS) {
            pthread_mutex_lock(&mux[c]);
            color_rand = tank[c].color;
            pthread_mutex_unlock(&mux[c]);
            color = color_rand;
        } else
            color = PINKYRED;

        sprintf(t, "%c", s[c]);
        textout_ex(animated_buffer, font, t,
                   TANKSIM_X + COL1_OFFSET / 2 + CHAR_SPACING * c,
                   TANKSIM_Y - 1.5 * WINDOW_OFFSET, color, -1);
    }
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_DM_TAB:    Draw deadline misses / response times tab
                    Graphics: [static]
_____________________________________________________________________________*/

void draw_dm_tab()
{
    int  j;           // task index
    char s_task[20];  // task name string

    // Draw tab borders (pinkyred color)
    rect(static_buffer, DM_X, DM_Y, DM_X + DM_WIDTH, DM_Y + DM_HEIGHT,
         PINKYRED);

    // Draw horizontal line above "Process" (pinkyred color)
    line(static_buffer, DM_X, DM_Y + DM_HEIGHT - ROW_OFFSET + 1,
         DM_X + DM_WIDTH, DM_Y + DM_HEIGHT - ROW_OFFSET + 1, PINKYRED);

    // Write tab field names
    textout_ex(static_buffer, font, "Tasks", DM_X + COL1_OFFSET,
               DM_Y - 1.5 * WINDOW_OFFSET, PINKYRED, -1);
    textout_ex(static_buffer, font, "DMs", DM_X + COL2_OFFSET,
               DM_Y - 1.5 * WINDOW_OFFSET, PINKYRED, -1);

    // Write tasks names
    for (j = 0; j < 2 * NUM_TANKS + 2; j++) {
        if (j < NUM_TANKS)
            sprintf(s_task, "Tank %d", j + 1);
        else if (j < 2 * NUM_TANKS)
            sprintf(s_task, "Sensor %d", j + 1 - NUM_TANKS);
        else if (j == 2 * NUM_TANKS)
            sprintf(s_task, "User");
        else if (j == 2 * NUM_TANKS + 1)
            sprintf(s_task, "Graphics");
        textout_ex(static_buffer, font, s_task, DM_X + COL1_OFFSET,
                   DM_Y + WINDOW_OFFSET + j * ROW_OFFSET, GREEN, -1);
    }
    sprintf(s_task, "PROCESS");
    textout_ex(static_buffer, font, s_task, DM_X + COL1_OFFSET,
               DM_Y + WINDOW_OFFSET + (NUM_TASKS)*ROW_OFFSET, GREEN, -1);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    WRITE_DM:   Write the number of deadline misses (dm) for the j-th task
                Graphics: [animated]
_____________________________________________________________________________*/

void write_dm(int j, int dm)
{
    char s_dm[20];  // deadline misses count string

    sprintf(s_dm, "%d", dm);
    textout_ex(animated_buffer, font, s_dm, DM_X + COL2_OFFSET,
               DM_Y + WINDOW_OFFSET + j * ROW_OFFSET, WHITE, -1);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_UTIL_BARS: Draw utilization bars for the j-th task:
                        * instantaneaus utilization: red bar
                        * maximum utilization: red line
                        * average utilization: blue bar
                    Graphics: [animated]
_____________________________________________________________________________*/

void draw_util_bars(int j, double util_scale_local, double util_inst,
                    double util_inst_max, double util_avg)
{
    int    bar_max_length;   // possible max length for utilization bars [pixel]
    int    util_inst_p;      // instantaneous utilization factor         [pixel]
    int    util_inst_max_p;  // max. instantaneous utilization factor    [pixel]
    int    util_avg_p;       // average utilization factor               [pixel]
    double percentage;       // percentage currently zoomed
    char   s_percentage[20];  // string of the percentage currently zoomed

    bar_max_length = DM_WIDTH - 1.5 * COL2_OFFSET;

    // Scale utilization factors (by util_scale)
    util_inst     = util_inst * (double)util_scale_local;
    util_inst_max = util_inst_max * (double)util_scale_local;
    util_avg      = util_avg * (double)util_scale_local;

    // Percentage to pixel conversions
    util_inst_p     = (int)round(util_inst * (double)bar_max_length);
    util_inst_max_p = (int)round(util_inst_max * (double)bar_max_length);
    util_avg_p      = (int)round(util_avg * (double)bar_max_length);

    // Don't make the utilization bars exceed the tab rightmost border
    if (util_inst_p >= bar_max_length - 1)
        util_inst_p = bar_max_length - 1;
    if (util_inst_max_p >= bar_max_length - 1)
        util_inst_max_p = bar_max_length - 1;
    if (util_avg_p >= bar_max_length - 1)
        util_avg_p = bar_max_length - 1;

    // Draw instantaneous utilization factor (red bars)
    rectfill(animated_buffer, DM_X + 1.5 * COL2_OFFSET - 0,
             DM_Y + WINDOW_OFFSET + (j)*ROW_OFFSET - 1,
             DM_X + 1.5 * COL2_OFFSET + util_inst_p,
             DM_Y + WINDOW_OFFSET + (j + 0.2) * ROW_OFFSET - 1, RED);

    // Draw maximum instantaneous utilization factor (red line)
    rectfill(animated_buffer, DM_X + 1.5 * COL2_OFFSET + util_inst_max_p,
             DM_Y + WINDOW_OFFSET + (j)*ROW_OFFSET - 1,
             DM_X + 1.5 * COL2_OFFSET + util_inst_max_p,
             DM_Y + WINDOW_OFFSET + (j + 0.2) * ROW_OFFSET - 1, RED);

    // Draw average utilization factor (blue bars)
    rectfill(animated_buffer, DM_X + 1.5 * COL2_OFFSET - 0,
             DM_Y + WINDOW_OFFSET + (j + 0.35) * ROW_OFFSET - 1,
             DM_X + 1.5 * COL2_OFFSET + util_avg_p,
             DM_Y + WINDOW_OFFSET + (j + 0.5) * ROW_OFFSET - 1, AZURE);

    // Draw zoom percentage (bottom-right, in orange)
    percentage = PERCENT / util_scale_local;
    sprintf(s_percentage, "%10.1f %%", percentage);
    textout_centre_ex(animated_buffer, font, s_percentage,
                      DM_X + DM_WIDTH - 3 * WINDOW_OFFSET,
                      DM_Y + DM_HEIGHT + WINDOW_OFFSET / 2, ORANGE, -1);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_UTIL_BARS: Draw maximum possible utilization bar (for the entire
                    process); green line
                    Graphics: [animated]
_____________________________________________________________________________*/

void draw_max_possible_util_bar(int j, double util_scale_local,
                                double util_max_process)
{
    int bar_max_length;      // possible max length for utilization bars [pixel]
    int util_max_process_p;  // max possible utilization factor          [pixel]

    bar_max_length = DM_WIDTH - 1.5 * COL2_OFFSET;

    // Scale utilization factors (by util_scale)
    util_max_process = util_max_process * (double)util_scale_local;

    // Percentage to pixel conversions
    util_max_process_p = (int)round(util_max_process * (double)bar_max_length);

    // Don't make the utilization bars exceed the tab rightmost border
    if (util_max_process_p >= bar_max_length - 1)
        util_max_process_p = bar_max_length - 1;

    // Draw maximum possible utilization factor (green line)
    rectfill(animated_buffer, DM_X + 1.5 * COL2_OFFSET + util_max_process_p,
             DM_Y + WINDOW_OFFSET + (j)*ROW_OFFSET - 1,
             DM_X + 1.5 * COL2_OFFSET + util_max_process_p,
             DM_Y + WINDOW_OFFSET + (j + 0.5) * ROW_OFFSET - 1, GREEN);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    UPDATE_DM_TAB:  For each task (and eventually, for the entire process),
                    write its (updated) deadline miss count and draw its
                    (updated) utilizations
                    Graphics: [animated]
_____________________________________________________________________________*/

void update_dm_tab()
{
    int    j;                  // task index
    double util_scale_local;   // scale factor of the utilization factors
    double util_inst;          // instantaneous utilization factor          [%]
    double util_inst_max;      // maximum instantaneous utilization factor  [%]
    double util_avg;           // average utilization factor                [%]
    int    dm;                 // task deadline miss counter
    char   s[30];              // util. factor (with variable scale factor) name
    double util_inst_process;  // process inst. utilization factor          [%]
    double util_avg_process;   // process maximum inst. utilization factor  [%]
    int    dm_process;         // deadline misses of the process
    double util_max_process;   // summation of all the max. inst. utilizations

    util_inst_process = 0;
    util_avg_process  = 0;
    dm_process        = 0;
    util_max_process  = 0;

    pthread_mutex_lock(&mux_util);
    util_scale_local = util_scale;
    pthread_mutex_unlock(&mux_util);
    util_max_process = 0;

    /* Write a tab field name (with the current value of the utilization scale
       factor) */
    if (util_scale_local == U_MIN)
        sprintf(s, "Response Times (%%,x%.1f)", util_scale_local);
    else
        sprintf(s, "Response Times (%%,x%i)", (int)util_scale_local);
    textout_ex(animated_buffer, font, s, DM_X + 1.5 * COL2_OFFSET - 5,
               DM_Y - 1.5 * WINDOW_OFFSET, PINKYRED, -1);

    // A) For each j-th task:
    for (j = 0; j < NUM_TASKS; j++) {
        // 1) Write its deadline misses
        dm = task_get_deadline_miss(j);
        dm_process += dm;
        write_dm(j, dm);

        // 2) Draw its utilization bars
        util_inst     = task_get_util_inst(j);
        util_inst_max = task_get_util_inst_max(j);
        util_avg      = task_get_util_avg(j);
        util_inst_process += util_inst;
        util_max_process += util_inst_max;
        util_avg_process += util_avg;
        draw_util_bars(j, util_scale_local, util_inst, util_inst_max, util_avg);
    }

    // B) Process:
    j = NUM_TASKS;

    // 1) Write process deadline misses
    dm = dm_process;
    write_dm(j, dm);

    // 2) Draw process utilization bars
    util_inst_max_process = fmax(util_inst_process, util_inst_max_process);
    util_inst             = util_inst_process;
    util_inst_max         = util_inst_max_process;
    util_avg              = util_avg_process;
    draw_util_bars(j, util_scale_local, util_inst, util_inst_max, util_avg);
    draw_max_possible_util_bar(j, util_scale_local, util_max_process);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    PRINT_COMPANY_LOGO: Print company's name (bottom-right, in azure)
                        Graphics: [animated]
_____________________________________________________________________________*/

void print_company_logo()
{
    char s[50];     // company's name
    int  length_s;  // character (of the the string s) counter

    sprintf(s, "© 2021 REAL-TIME INDUSTRY Inc.");

    for (length_s = 0; s[length_s] != '\0'; ++length_s)
        ;

    textout_ex(static_buffer, font, s, RES_X - length_s * CHAR_SPACING / 2,
               RES_Y - ROW_OFFSET, AZURE, -1);
}


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_RANDOMIZER_ELLIPSE:    Draw an ellipse for creating the randomizer
                                button, colored:
                                    * WHITE, if the toggle is "on"
                                    * AZURE, if the toggle is "off"
                                Graphics: [animated] (despite static_buffer is
                                used)
_____________________________________________________________________________*/

void draw_randomizer_ellipse(int toggle)
{
    int border_left;    // randomizer button left border coordinate
    int border_right;   // randomizer button right border coordinate
    int border_bottom;  // randomizer button bottom border coordinate
    int border_top;     // randomizer button top border coordinate
    int color;          // ellipse color

    border_left =
        TANKSIM_X + TANKSIM_WIDTH - 11 * CHAR_SPACING + 0 * CHAR_SPACING;
    border_right =
        TANKSIM_X + TANKSIM_WIDTH - 10 * CHAR_SPACING + 10 * CHAR_SPACING;
    border_bottom = COMMANDS_Y - 0.5 * WINDOW_OFFSET;
    border_top    = COMMANDS_Y - 2.5 * WINDOW_OFFSET;

    if (toggle == ON)
        color = WHITE;
    else if (toggle == OFF)
        color = AZURE;
    else {
        fprintf(stderr, "Wrong toggle value (%i) for the randomizer button.\n",
                toggle);
        exit(EXIT_FAILURE);
    }

    ellipsefill(static_buffer, (border_right + border_left) / 2,
                (border_bottom + border_top) / 2,
                (border_right - border_left) / 2,
                (border_bottom - border_top) / 2, color);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_RANDOMIZER_BUTTON: Just write "R@NDOMIZE!" (in pinkyred ) on the
                            randomizer button
                            Graphics: [animated]
_____________________________________________________________________________*/

void draw_randomizer_button()
{
    char s[20];     // button name string
    int  c;         // character index
    char t[20];     // c-th char of the button name string
    int  length_s;  // character (of the string s) counter

    sprintf(s, "R@NDOMIZE!");

    for (length_s = 0; s[length_s] != '\0'; ++length_s)
        ;

    for (c = 0; c < length_s; c++) {
        sprintf(t, "%c", s[c]);
        textout_ex(animated_buffer, font, t,
                   TANKSIM_X + TANKSIM_WIDTH - length_s * CHAR_SPACING +
                       c * CHAR_SPACING,
                   COMMANDS_Y - 1.8 * WINDOW_OFFSET, PINKYRED, -1);
    }
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    UPDATE_TANKS_TAB:   Draw all the tanks. In more detail:
                            *) their borders
                            *) their water
                            *) their measured value
                            *) their desired value (with a small fader)
                            *) their inlet knob
                            *) their outlet knob
                        Graphics: [animated]
_____________________________________________________________________________*/

void update_tanks_tab()
{
    int    i;  // tank index
    double height, r, h, h_meas0, h_des, phi_in,
        phi_out;      // tank float parameters
    int x, y, color;  // tank integer parameters

    pthread_mutex_lock(&mux_dim);
    height = dim.height;
    r      = dim.r;
    pthread_mutex_unlock(&mux_dim);

    // Draw the i-th tank
    for (i = 0; i < NUM_TANKS; i++) {
        pthread_mutex_lock(&mux[i]);
        x       = tank[i].pos.x;
        y       = tank[i].pos.y;
        h       = tank[i].lev.h;
        h_meas0 = tank[i].lev.h_meas0;
        h_des   = tank[i].lev.h_des;
        phi_in  = tank[i].phi.in;
        phi_out = tank[i].phi.out;
        color   = tank[i].color;
        pthread_mutex_unlock(&mux[i]);

        draw_tank_borders(x, y, height, r, color);
        draw_water(x, y, height, r, h);
        write_h_meas_slider_value(x, y, height, r, h_meas0);
        draw_h_des_line(x, y, height, r, h_des);
        write_h_des_slider_value(x, y, height, r, h_des);
        draw_in_knob(x, y, height);
        update_in_knob(x, y, height, phi_in);
        draw_out_knob(x, y);
        update_out_knob(x, y, phi_out);
    }
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DRAW_STATIC_GRAPHICS:   Draw static graphics once and for all
                            Graphics: [static]
_____________________________________________________________________________*/

void draw_static_graphics()
{
    draw_commands_tab();
    draw_parameters_tab();
    draw_dm_tab();
    draw_randomizer_ellipse(OFF);
    print_company_logo();
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INIT_GRAPHICS_BUFFERS:  Initialize graphics buffers
_____________________________________________________________________________*/

void init_graphics_buffers()
{
    // Create two empty bitmaps
    static_buffer   = create_bitmap(RES_X, RES_Y);
    animated_buffer = create_bitmap(RES_X, RES_Y);

    if (!static_buffer) {
        fprintf(stderr, "Couldn't create bitmap (static_buffer)!\n");
        exit(EXIT_FAILURE);
    }
    if (!static_buffer) {
        fprintf(stderr, "Couldn't create bitmap (animated_buffer)!\n");
        exit(EXIT_FAILURE);
    }

    clear_to_color(static_buffer, BLACK);
    clear_to_color(animated_buffer, BLACK);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    GRAPHICS_TASK:  Body of graphics task
_____________________________________________________________________________*/

void *graphics_task(void *arg)
{
    int             j;         // task index (graphics task)
    struct timespec t1;        // t1:   time before task execution
    struct timespec t2;        // t2:   time after task execution
    double          rt_value;  // response_time[j] = t2 - t1                [ms]
    unsigned long   m;         // task execution counter
    char            s[50];     // for printing task name on the terminal

    // 1) Static graphics (to be drawn once)
    /* draw_static_graphics() is already called in `init.c`; no need to execute
       it again */

    m = 0;
    j = task_get_index(arg);
    task_set_activation(j);

    // 2) Animated graphics (to be drawn at each execution)
    while (!end) {
        clock_gettime(CLOCK_MONOTONIC, &t1);  // t1

        // 2a) Clear screen
        clear(animated_buffer);

        // 2b) Everything in the static buffer is moved to the animated buffer
        blit(static_buffer, animated_buffer, 0, 0, 0, 0, RES_X, RES_Y);

        // 2c) Compute animated graphics
        draw_randomizer_button();
        draw_tanks_tab();
        update_parameters_tab();
        update_dm_tab();
        update_tanks_tab();

        // 2d) Everything in the animated buffer is finally moved to the screen
        // scare_mouse();   // useless, since hardware cursor is being used
        blit(animated_buffer, screen, 0, 0, 0, 0, RES_X, RES_Y);
        // unscare_mouse(); // useless, since hardware cursor is being used

        clock_gettime(CLOCK_MONOTONIC, &t2);       // t2
        rt_value = get_time_diff_in_ms(&t2, &t1);  // t2 - t1
        manage_rt(j, m, rt_value);
        m++;

        task_check_deadline_miss(j);
        task_wait_for_period(j);
    }

    if (WOET == ON) {  // Print (some of the) response times on terminal
        sprintf(s, "************  Graphics Task   ************\n");
        print_dm_rt(j, s);
    }

    return 0;
}
//-----------------------------------------------------------------------------