#include "step_motor.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

#define OPT_SENSOR_PIN 28
#define MOTOR_PINS     {2, 3, 6, 13}
#define CALIB_CYCLES   3

// Half‑step drive sequence for the stepper
static const int8_t halfstep[8][4] = {
    {1,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,1,1,0},
    {0,0,1,0}, {0,0,1,1}, {0,0,0,1}, {1,0,0,1}
};

// Module‑private state
static bool calibrated = false;
static int steps_per_rev = 0;
static int current_step = 0;

// Initialize stepper pins and optical sensor
void init_motor(void) {
    const int pins[4] = MOTOR_PINS;
    for (int i = 0; i < 4; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
    }
    gpio_init(OPT_SENSOR_PIN);
    gpio_set_dir(OPT_SENSOR_PIN, GPIO_IN);
    gpio_pull_up(OPT_SENSOR_PIN);
}

// Advance one half‑step
void step_motor(int direction) {
    current_step = (current_step + direction + 8) % 8;
    const int pins[4] = MOTOR_PINS;
    if (direction >= 0){
        for (int i = 0; i < 4; i++) {
            gpio_put(pins[i], halfstep[current_step][i]);
        }
    }else{
        for (int i = 3; i >= 0; i--) {
            gpio_put(pins[i], halfstep[current_step][i]);
        }
    }

    sleep_ms(1);
}

// Calibrate full revolution via optical sensor
void reverse_calibrate_wheel(int pill_count, int steps){
    printf("Reverse calibrating wheel (no steps counted)...\n");
    int count = 0;

    while (gpio_get(OPT_SENSOR_PIN)) { step_motor(-1); count++; }
    //while (!gpio_get(OPT_SENSOR_PIN)) { step_motor(-1); count++; }

    printf("Rever   se calibration steps: %d\n", count);
    steps_per_rev = steps;
    for (int i = 0; i < pill_count*(steps/8); i++) {
        step_motor(1);
    }
    calibrated = true;
}

int calibrate_wheel(void) {
    int total = 0;
    int opt = 0;
    printf("Calibrating wheel...\n");

    // Align to first edge
    while (gpio_get(OPT_SENSOR_PIN)) step_motor(1);
    while (!gpio_get(OPT_SENSOR_PIN)) step_motor(1);

    for (int run = 0; run < CALIB_CYCLES; run++) {
        int count = 0;
        while (gpio_get(OPT_SENSOR_PIN)) { step_motor(1); count++; }
        while (!gpio_get(OPT_SENSOR_PIN)) { step_motor(1); count++; opt++; }
        printf("  Run %d: %d steps\n", run+1, count);
        if (run > 0) total += count;
    }
    steps_per_rev = total / (CALIB_CYCLES - 1);
    int ok = steps_per_rev - (150);
    while (ok > 0)
    {
        step_motor(1);
        ok--;
    }
    printf("Calibrated: %d steps/rev\n", steps_per_rev);

    // Reset position
    current_step = 0;
    step_motor(0);
    calibrated = true;
    return steps_per_rev;
}

// Move forward by “slots” (1 slot = 1/8 revolution)
void run_motor(int slots) {
    if (!calibrated) {
        printf("ERROR: wheel not calibrated!\n");
        return;
    }
    int step_count = (steps_per_rev / 8) * slots;
    for (int i = 0; i < step_count; i++) {
        step_motor(1);
    }
}
