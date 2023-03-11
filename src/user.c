//*****************************************************************************
//***************** USER.C - Manage mouse and keyboard events *****************
//*****************           Author: Livio Bisogni           *****************
//*****************************************************************************

/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
                                  INSTRUCTIONS

    Please read the attached `README.md` file.
_____________________________________________________________________________*/


#include "user.h"
#include <allegro.h>
#include <math.h>
#include <stdio.h>
#include "graphics.h"
#include "init.h"
#include "easy_pthread_task.h"
#include "easy_pthread_time.h"
#include "tank.h"


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    ROUND_FIRST_DECIMALS:   Given a floating number fl_num, round it to
                            decimal_digits decimal places
_____________________________________________________________________________*/

double round_first_decimals(double fl_num, int decimal_digits)
{
    assert(decimal_digits >= 0);
    double nearest;
    int    power_of_ten;  // 10^decimal_digits

    power_of_ten = pow(10, decimal_digits);
    nearest      = round(fl_num * power_of_ten) / power_of_ten;

    return nearest;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    FRAND:  Return a random double in [xmi, xma)
_____________________________________________________________________________*/

double frand(double xmi, double xma)
{
    double r;
    r = rand() / (double)RAND_MAX;  // random double in [0,1)
    return xmi + (xma - xmi) * r;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    GET_ASCIICODE:  Return the ASCII code of a pressed key
_____________________________________________________________________________*/

char get_asciicode()
{
    if (keypressed()) {
        return readkey() & 0xff;
    } else
        return 0;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    CHECK_MOUSE_ON_BOTTOM_KNOB: Return 1 if mouse (at position (m_x, m_y)) is
                                over the outlet knob of the i-th tank; 0
                                otherwise
_____________________________________________________________________________*/

int check_mouse_on_bottom_knob(int m_x, int m_y, int i)
{
    int x, y;                // tank integer parameters                 [pixel]
    int x_circle, y_circle;  // outlet knob (circle) position           [pixel]
    int r_circle_p;          // circle radius                           [pixel]

    pthread_mutex_lock(&mux[i]);
    x = tank[i].pos.x;
    y = tank[i].pos.y;
    pthread_mutex_unlock(&mux[i]);

    r_circle_p = m2p(CIRCLE_RADIUS);
    x_circle   = x;
    y_circle   = y + r_circle_p + YCIRCLE_OFFSET;

    if (sqrt(pow((double)m_x - (double)x_circle, 2) +
             pow((double)m_y - (double)y_circle, 2)) <= r_circle_p)
        return 1;
    else
        return 0;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    MANAGE_MOUSE_ON_BOTTOM_KNOB:    Move i-th outlet knob according to mouse
                                    position (m_x, m_y)
_____________________________________________________________________________*/

void manage_mouse_on_bottom_knob(int m_x, int m_y, int i)
{
    int x_circle, y_circle;  // outlet knob (circle) position           [pixel]
    int x, y;                // tank integer parameters                 [pixel]
    int x_rel, y_rel;        /* (m_x, m_y) mouse position with respect  [pixel]
                                to outlet knob center */
    double phi_knob;         /* outlet valve openness; value in [0, 1]  [pixel]
                                It's the phi_out obtained moving the
                                corresponding outlet knob */
    int r_knob_p;            // outlet knob radius                      [pixel]

    pthread_mutex_lock(&mux[i]);
    x = tank[i].pos.x;
    y = tank[i].pos.y;
    pthread_mutex_unlock(&mux[i]);

    r_knob_p = m2p(CIRCLE_RADIUS);

    x_circle = x;
    y_circle = y + r_knob_p + YCIRCLE_OFFSET;

    x_rel = x_circle - m_x;
    y_rel = y_circle - m_y;

    // Compute outlet knob value (based on mouse position while being held down)
    phi_knob = (atan2((double)y_rel, (double)x_rel) + PI) / (2 * PI) - 0.25;

    // Constrain the outlet valve position to stay in [0, 1]
    if (phi_knob >= 0 && phi_knob <= 0.01)
        phi_knob = 0;
    if (phi_knob < 0 && phi_knob >= -0.25)
        phi_knob = 1 - fabs(phi_knob);
    phi_knob = round_first_decimals(phi_knob, PHI_DIGITS);

    pthread_mutex_lock(&mux[i]);
    tank[i].phi.out = phi_knob;
    pthread_mutex_unlock(&mux[i]);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    CHECK_MOUSE_ON_TANK:    Return 1 if mouse (at position (m_x, m_y)) is over
                            the i-th tank (of given radius and height); 0
                            otherwise
_____________________________________________________________________________*/

int check_mouse_on_tank(int m_x, int m_y, double height, int r_p, int i)
{
    int x, y;   // tank integer parameters                              [pixel]
    int cond1;  // limit inferior for m_x
    int cond2;  // limit superior for m_x
    int cond3;  // limit inferior for m_y
    int cond4;  // limit superior for m_y

    pthread_mutex_lock(&mux[i]);
    x = tank[i].pos.x;
    y = tank[i].pos.y;
    pthread_mutex_unlock(&mux[i]);

    cond1 = (m_x >= x - r_p - MAX_TANK_BORDER_THICKNESS);
    cond2 = (m_x <= x + r_p + MAX_TANK_BORDER_THICKNESS);
    cond3 = (m_y <= y + MAX_TANK_BORDER_THICKNESS);
    cond4 = (m_y >= y - m2p(height) - MAX_TANK_BORDER_THICKNESS);

    if (cond1 && cond2 && cond3 && cond4)
        return 1;
    return 0;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    MANAGE_MOUSE_ON_TANK:   Move i-th tank (of given height) desired water
                            fader according to mouse position (m_x, m_y)
_____________________________________________________________________________*/

void manage_mouse_on_tank(int m_x, int m_y, double height, int i)
{
    double h_des;  // desired water level                                   [m]
    int    y;      // tank y coordinate                                 [pixel]

    pthread_mutex_lock(&mux[i]);
    y = tank[i].pos.y;
    pthread_mutex_unlock(&mux[i]);

    h_des = p2m(y - m_y);

    // Constrain the desired water level to stay in [0, height]
    if (h_des < 0)
        h_des = 0;
    if (h_des > height)
        h_des = height;

    pthread_mutex_lock(&mux[i]);
    tank[i].lev.h_des = h_des;
    pthread_mutex_unlock(&mux[i]);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    CHECK_MOUSE_ON_RANDOM_BUTTON:   Check whether the mouse is on the randomizer
                                    button: return 1 if mouse (at position (m_x,
                                    m_y)) is over the randomizer button; 0
                                    otherwise
_____________________________________________________________________________*/

int check_mouse_on_random_button(int m_x, int m_y)
{
    int border_left;    // randomizer button left border coordinate     [pixel]
    int border_right;   // randomizer button right border coordinate    [pixel]
    int border_bottom;  // randomizer button bottom border coordinate   [pixel]
    int border_top;     // randomizer button top border coordinate      [pixel]

    border_left =
        TANKSIM_X + TANKSIM_WIDTH - 11 * CHAR_SPACING + 0 * CHAR_SPACING;
    border_right =
        TANKSIM_X + TANKSIM_WIDTH - 10 * CHAR_SPACING + 10 * CHAR_SPACING;
    border_bottom = COMMANDS_Y - 0.5 * WINDOW_OFFSET;
    border_top    = COMMANDS_Y - 2.5 * WINDOW_OFFSET;

    if (m_x >= border_left && m_x <= border_right && m_y <= border_bottom &&
        m_y >= border_top)
        return 1;
    return 0;
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    MANAGE_MOUSE_ON_RANDOM_BUTTON:  Once the mouse clicks the randomizer button,
                                    for each tank:
                                        1) randomize its desired water level
                                        2) randomize its outlet spool position
                                        3) randomize its borders color
_____________________________________________________________________________*/

void manage_mouse_on_random_button()
{
    int    i;                 // tank index
    double height;            // tanks height                               [m]
    int    red, green, blue;  // primary colors; pseudo-randomly generated
    int    color_rand;        // = (red, green, blue)

    pthread_mutex_lock(&mux_dim);
    height = dim.height;
    pthread_mutex_unlock(&mux_dim);

    for (i = 0; i < NUM_TANKS; i++) {
        // Generate random color
        red        = rand() % 256;
        green      = rand() % 256;
        blue       = rand() % 256;
        color_rand = makecol(red, green, blue);

        pthread_mutex_lock(&mux[i]);
        tank[i].lev.h_des =
            quantize(frand(0, height));  // 1) randomized (and quantized) h_des
        tank[i].phi.out = frand(0, 1);   // 2) randomized phi_out
        tank[i].color   = color_rand;    // 3) randomized color
        pthread_mutex_unlock(&mux[i]);
    }
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INCREASE_KP:    Increase proportional gain Kp (when 'P' is pressed)
_____________________________________________________________________________*/

static void increase_kp()
{
    double kp;

    pthread_mutex_lock(&mux_pid);
    kp = pid.kp;
    if (round_first_decimals(kp, KP_DIGITS) <= KP_MAX - KP_STEPSIZE) {
        kp     = round_first_decimals(kp + KP_STEPSIZE, KP_DIGITS);
        pid.kp = kp;
    }
    pthread_mutex_unlock(&mux_pid);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DECREASE_KP:    Decrease proportional gain Kp (when 'p' is pressed)
_____________________________________________________________________________*/

void decrease_kp()
{
    double kp;

    pthread_mutex_lock(&mux_pid);
    kp = pid.kp;
    if (round_first_decimals(kp, KP_DIGITS) >= KP_MIN + KP_STEPSIZE) {
        kp     = round_first_decimals(kp - KP_STEPSIZE, KP_DIGITS);
        pid.kp = kp;
    }
    pthread_mutex_unlock(&mux_pid);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INCREASE_KI:    Increase integral gain Ki (when 'I' is pressed)
_____________________________________________________________________________*/

void increase_ki()
{
    double ki;

    pthread_mutex_lock(&mux_pid);
    ki = pid.ki;
    if (round_first_decimals(ki, KI_DIGITS) <= KI_MAX - KI_STEPSIZE) {
        ki     = round_first_decimals(ki + KI_STEPSIZE, KI_DIGITS);
        pid.ki = ki;
    }
    pthread_mutex_unlock(&mux_pid);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DECREASE_KI:    Decrease integral gain Ki (when 'i' is pressed)
_____________________________________________________________________________*/

void decrease_ki()
{
    double ki;

    pthread_mutex_lock(&mux_pid);
    ki = pid.ki;
    if (round_first_decimals(ki, KI_DIGITS) >= KI_MIN + KI_STEPSIZE) {
        ki     = round_first_decimals(ki - KI_STEPSIZE, KI_DIGITS);
        pid.ki = ki;
    }
    pthread_mutex_unlock(&mux_pid);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INCREASE_KD:    Increase derivative gain Kd (when 'D' is pressed)
_____________________________________________________________________________*/

void increase_kd()
{
    double kd;

    pthread_mutex_lock(&mux_pid);
    kd = pid.kd;
    if (round_first_decimals(kd, KD_DIGITS) <= KD_MAX - KD_STEPSIZE) {
        kd     = round_first_decimals(kd + KD_STEPSIZE, KD_DIGITS);
        pid.kd = kd;
    }
    pthread_mutex_unlock(&mux_pid);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DECREASE_KD:    Decrease derivative gain Kd (when 'd' is pressed)
_____________________________________________________________________________*/

void decrease_kd()
{
    double kd;

    pthread_mutex_lock(&mux_pid);
    kd = pid.kd;
    if (round_first_decimals(kd, KD_DIGITS) >= KD_MIN + KD_STEPSIZE) {
        kd     = round_first_decimals(kd - KD_STEPSIZE, KD_DIGITS);
        pid.kd = kd;
    }
    pthread_mutex_unlock(&mux_pid);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INCREASE_F: Increase feedforward f (wetter) (when 'F' is pressed)
_____________________________________________________________________________*/

void increase_f()
{
    double f;

    pthread_mutex_lock(&mux_pid);
    f = pid.f;
    if (round_first_decimals(f, 2) <= F_MAX - F_STEPSIZE) {
        f     = round_first_decimals(f + F_STEPSIZE, 2);
        pid.f = f;
    }
    pthread_mutex_unlock(&mux_pid);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DECREASE_F: Decrease feedforward f (drier) (when 'f' is pressed)
_____________________________________________________________________________*/

void decrease_f()
{
    double f;

    pthread_mutex_lock(&mux_pid);
    f = pid.f;
    if (round_first_decimals(f, 2) >= F_MIN + F_STEPSIZE) {
        f     = round_first_decimals(f - F_STEPSIZE, 2);
        pid.f = f;
    }
    pthread_mutex_unlock(&mux_pid);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INCREASE_W: Increase window size (aka filter length) w (when 'W' is pressed)
_____________________________________________________________________________*/

void increase_w()
{
    int w_local;

    pthread_mutex_lock(&mux_w);
    w_local = w;
    if (w_local <= W_MAX - W_STEPSIZE) {
        w_local = w_local + W_STEPSIZE;
        w       = w_local;
    }
    pthread_mutex_unlock(&mux_w);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DECREASE_W: Decrease window size (aka filter length) w (when 'w' is pressed)
_____________________________________________________________________________*/

void decrease_w()
{
    int w_local;

    pthread_mutex_lock(&mux_w);
    w_local = w;
    if (w_local >= W_MIN + W_STEPSIZE) {
        w_local = w_local - W_STEPSIZE;
        w       = w_local;
    }
    pthread_mutex_unlock(&mux_w);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INCREASE_K: Increase valve constant k (when 'K' is pressed)
_____________________________________________________________________________*/

void increase_k()
{
    double k_local;

    pthread_mutex_lock(&mux_k);
    k_local = k;
    if (round_first_decimals(k_local, 1) <= K_MAX - K_STEPSIZE) {
        k_local = round_first_decimals(k_local + K_STEPSIZE, 1);
        k       = k_local;
    }
    pthread_mutex_unlock(&mux_k);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DECREASE_K: Decrease valve constant k (when 'k' is pressed)
_____________________________________________________________________________*/

void decrease_k()
{
    double k_local;

    pthread_mutex_lock(&mux_k);
    k_local = k;
    if (round_first_decimals(k_local, 1) >= K_MIN + K_STEPSIZE) {
        k_local = round_first_decimals(k_local - K_STEPSIZE, 1);
        k       = k_local;
    }
    pthread_mutex_unlock(&mux_k);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INCREASE_R: Increase tanks radius r (when 'R' is pressed)
_____________________________________________________________________________*/

void increase_r()
{
    double r;

    pthread_mutex_lock(&mux_dim);
    r = dim.r;
    if (round_first_decimals(r, 1) <= R_MAX - R_STEPSIZE) {
        r     = round_first_decimals(r + R_STEPSIZE, 1);
        dim.r = r;
    }
    pthread_mutex_unlock(&mux_dim);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DECREASE_R: Decrease tanks radius r (when 'r' is pressed)
_____________________________________________________________________________*/

void decrease_r()
{
    double r;

    pthread_mutex_lock(&mux_dim);
    r = dim.r;
    if (round_first_decimals(r, 1) >= R_MIN + R_STEPSIZE) {
        r     = round_first_decimals(r - R_STEPSIZE, 1);
        dim.r = r;
    }
    pthread_mutex_unlock(&mux_dim);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INCREASE_H: Increase tanks height (when 'H' is pressed)
_____________________________________________________________________________*/

void increase_h()
{
    double height;

    pthread_mutex_lock(&mux_dim);
    height = dim.height;
    if (round_first_decimals(height, 2) <= HEIGHT_MAX - HEIGHT_STEPSIZE) {
        height     = round_first_decimals(height + HEIGHT_STEPSIZE, 2);
        dim.height = height;
    }
    pthread_mutex_unlock(&mux_dim);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DECREASE_H: Decrease tanks height (when 'h' is pressed)
_____________________________________________________________________________*/

void decrease_h()
{
    double height;  // tanks height
    double h_des;   // tank desired water level
    int    i;       // tank index

    pthread_mutex_lock(&mux_dim);
    height = dim.height;
    pthread_mutex_unlock(&mux_dim);
    if (round_first_decimals(height, 2) >= HEIGHT_MIN + HEIGHT_STEPSIZE) {
        height = round_first_decimals(height - HEIGHT_STEPSIZE, 2);

        /* Don't allow the desired levels to be greater than the current
           tanks height */
        for (i = 0; i < NUM_TANKS; i++) {
            pthread_mutex_lock(&mux[i]);
            h_des = tank[i].lev.h_des;
            if (h_des > height) {
                tank[i].lev.h_des = height;
            }
            pthread_mutex_unlock(&mux[i]);
        }

        pthread_mutex_lock(&mux_dim);
        dim.height = height;
        pthread_mutex_unlock(&mux_dim);
    }
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    INCREASE_UTIL_SCALE:    Increase utilization scale factor, i.e. zoom in
                            (when 'U' is pressed)
_____________________________________________________________________________*/

void increase_util_scale()
{
    int util_scale_local;

    pthread_mutex_lock(&mux_util);
    util_scale_local = util_scale;
    if (util_scale_local <= U_MAX - U_STEPSIZE) {
        util_scale_local = util_scale_local + U_STEPSIZE;
        util_scale       = util_scale_local;
    }
    pthread_mutex_unlock(&mux_util);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    DECREASE_UTIL_SCALE:    Decrease utilization scale factor, i.e. zoom out
                            (when 'u' is pressed)
_____________________________________________________________________________*/

void decrease_util_scale()
{
    int util_scale_local;

    pthread_mutex_lock(&mux_util);
    util_scale_local = util_scale;
    if (util_scale_local >= U_MIN + U_STEPSIZE) {
        util_scale_local = util_scale_local - U_STEPSIZE;
        util_scale       = util_scale_local;
    } else if (util_scale_local > U_MIN)
        util_scale = U_MIN;
    pthread_mutex_unlock(&mux_util);
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    RESTORE_DEFAULT_VALUES: Restore simulator default values (when 'SPACE' is
                            pressed)
_____________________________________________________________________________*/

void restore_default_values()
{
    int i;  // tank index

    pthread_mutex_lock(&mux_pid);
    pid.kp = KP_DEFAULT;
    pid.ki = KI_DEFAULT;
    pid.kd = KD_DEFAULT;
    pid.f  = F_DEFAULT;
    pthread_mutex_unlock(&mux_pid);
    pthread_mutex_lock(&mux_k);
    k = K_DEFAULT;
    pthread_mutex_unlock(&mux_k);
    pthread_mutex_lock(&mux_dim);
    dim.height = HEIGHT_DEFAULT;
    dim.r      = R_DEFAULT;
    pthread_mutex_unlock(&mux_dim);
    pthread_mutex_lock(&mux_w);
    w = W_DEFAULT;
    pthread_mutex_unlock(&mux_w);

    // For each i-th tank:
    for (i = 0; i < NUM_TANKS; i++) {
        pthread_mutex_lock(&mux[i]);
        tank[i].lev.h_des = quantize(H_DES_DEFAULT);
        tank[i].phi.out   = PHI_OUT_DEFAULT;
        pthread_mutex_unlock(&mux[i]);
    }

    // For each j-th task:
    for (int j = 0; j < NUM_TASKS; j++) {
        task_set_rt_max(j, 0);         // Reset its maximum response time
        task_set_deadline_miss(j, 0);  // Reset its deadline miss count
    }

    // Reset the maximum utilization of the entire process
    reset_util_inst_max_process();
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    MANAGE_KEYBOARD:    Manage keyboard events: for some key pressed events,
                        vary the corresponding simulator parameter.
                        Possible keys:
                                    1)  p/P:    proportional gain
                                    2)  i/I:    integral gain
                                    3)  d/D:    derivative gain
                                    4)  f/F:    feedfoward dry/wet
                                    5)  w/W:    window size aka filter width
                                    6)  k/K:    valve constant
                                    7)  r/R:    radius
                                    8)  h/H:    height
                                    9)  u/U:    response time scale
                                    10) SPACE:  restore default values
_____________________________________________________________________________*/

void manage_keyboard()
{
    char scan;

    scan = get_asciicode();

    switch (scan) {
    // p/P (proportional gain)
    case 'P':
        increase_kp();
        break;
    case 'p':
        decrease_kp();
        break;

    // i/I (integral gain)
    case 'I':
        increase_ki();
        break;
    case 'i':
        decrease_ki();
        break;

    // d/D (derivative gain)
    case 'D':
        increase_kd();
        break;
    case 'd':
        decrease_kd();
        break;

    // f/F (feedfoward dry/wet)
    case 'F':
        increase_f();
        break;
    case 'f':
        decrease_f();
        break;

    // w/W (window size aka filter length)
    case 'W':
        increase_w();
        break;
    case 'w':
        decrease_w();
        break;

    // k/K (valve constant)
    case 'K':
        increase_k();
        break;
    case 'k':
        decrease_k();
        break;

    // r/R (tank radius)
    case 'R':
        increase_r();
        break;
    case 'r':
        decrease_r();
        break;

    // h/H (height)
    case 'H':
        increase_h();
        break;
    case 'h':
        decrease_h();
        break;

    // u/U (response time scale)
    case 'U':
        increase_util_scale();
        break;
    case 'u':
        decrease_util_scale();
        break;

    // SPACE
    case ' ':
        restore_default_values();
        break;

    default:
        break;
    }
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    MANAGE_MOUSE:   Handle mouse clicks & movements on:
                        1) tanks (for setting their desired water levels in [0,
                           height])
                        2) outlet knobs (for setting their positions
                           in [0, 100%])
                        3) randomizer button (for creating a little of chaos)
_____________________________________________________________________________*/

void manage_mouse(int last_button, int current_button)
{
    int    i;                  // tank index
    int    r_p;                // tank radius                           [pixel]
    int    m_x, m_y;           // mouse position                        [pixel]
    double height, r;          // tank float parameters                 [m]
    int    mouse_clicked_once; /* it is equal to 1 the first time the
                                  mouse left button is clicked; 0 in any
                                  other case (until it's released) */

    mouse_clicked_once = (current_button & 1) && !(last_button & 1);

    m_x = mouse_x;
    m_y = mouse_y;

    // If mouse left button is being held down:
    if (current_button & 1) {
        pthread_mutex_lock(&mux_dim);
        height = dim.height;
        r      = dim.r;
        pthread_mutex_unlock(&mux_dim);
        r_p = m2p(r);

        for (i = 0; i < NUM_TANKS; i++) {
            // 1) If mouse is on i-th tank slider
            if (check_mouse_on_tank(m_x, m_y, height, r_p, i) == 1)
                manage_mouse_on_tank(m_x, m_y, height, i);
            // 2) If mouse is on i-th tank bottom (aka outlet) knob
            if (check_mouse_on_bottom_knob(m_x, m_y, i) == 1)
                manage_mouse_on_bottom_knob(m_x, m_y, i);
        }
    }

    // Randomizer button:
    // If mouse left button is pressed (considering only its first click) ...
    if (mouse_clicked_once) {
        // ... and if it is on the randomizer button:
        if (check_mouse_on_random_button(m_x, m_y) == 1) {
            draw_randomizer_ellipse(ON);
            manage_mouse_on_random_button();
        }
    }
    // If mouse left button is released ...
    if (!current_button && last_button) {
        // ... while it is on the randomizer button:
        if (check_mouse_on_random_button(m_x, m_y) == 1)
            draw_randomizer_ellipse(OFF);
    }
}
//-----------------------------------------------------------------------------


/*‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
    USER_TASK:  Body of user task
_____________________________________________________________________________*/

void *user_task(void *arg)
{
    int             j;               // task index (user task)
    struct timespec t1;              // t1: time before task execution
    struct timespec t2;              // t2: time after task execution
    double          rt_value;        // response_time[j] = t2 - t1          [ms]
    int             current_button;  // current (k = 0) mouse button pressed
    int             last_button;     // previous (k = -1) mouse button pressed
    unsigned long   m;               // task execution counter
    char            s[50];           // for printing task name on the terminal

    current_button = 0;

    m = 0;
    j = task_get_index(arg);
    task_set_activation(j);

    // Constantly monitor keyboard and mouse events
    while (!end) {
        clock_gettime(CLOCK_MONOTONIC, &t1);  // t1

        manage_keyboard();
        last_button    = current_button;
        current_button = mouse_b;
        manage_mouse(last_button, current_button);

        clock_gettime(CLOCK_MONOTONIC, &t2);       // t2
        rt_value = get_time_diff_in_ms(&t2, &t1);  // t2 - t1
        manage_rt(j, m, rt_value);
        m++;

        task_check_deadline_miss(j);
        task_wait_for_period(j);
    }

    if (WOET == ON) {  // Print (some of the) response times on terminal
        sprintf(s, "************    User Task     ************\n");
        print_dm_rt(j, s);
    }

    return 0;
}
//-----------------------------------------------------------------------------