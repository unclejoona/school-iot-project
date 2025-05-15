#ifndef STEP_MOTOR_H
#define STEP_MOTOR_H

#include <stdbool.h>

// Initialize the stepper motor GPIOs and optical sensor pin
void init_motor(void);

// Step the motor one half‑step in the given direction (+1 or -1)
void step_motor(int direction);

// Do a multi‑run calibration to measure steps per full revolution
int calibrate_wheel(void);

//reverse to start up place
void reverse_calibrate_wheel(int pill_count, int steps);
// Rotate the wheel by the given number of “slots” (1 slot = 1/8 rev)
void run_motor(int slots);

#endif // STEP_MOTOR_H
