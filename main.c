#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <sys/unistd.h>
#include "lora.h"
#include "eeprom.h"
#include "step_motor.h"
#include "pico/util/queue.h"
#include "hardware/i2c.h"
#define PIEZO_PIN        27
#define LED_PIN          20
#define BUTTON_CALIB_PIN 9
#define BUTTON_START_PIN 8
#define BUTTON_EXTRA_PIN 7
#define EEPROM_ADDR 0x50

#define I2C_PORT i2c0
#define I2C_SDA_PIN 16
#define I2C_SCL_PIN 17
#define CALIBRATED 0x0001
#define MEMORY_STATE_ADDR 0x0002
#define PILL_COUNT_ADDR 0x0003
#define STE_POSITION_ADDR 0x0004
#define STEP_MEMORY_ADDR 0x0005

static queue_t events;
static void blink_led(int n) {
    for (int i = 0; i < n; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(200);
        gpio_put(LED_PIN, 0);
        sleep_ms(200);
    }
}

static bool detect_pill(void) {
    absolute_time_t timeout = make_timeout_time_ms(1000);

    while (!time_reached(timeout)) {
        int value;
        queue_try_remove(&events, &value);
        if (value == 1) {
            while (queue_try_remove(&events, &value)) {}
            return true;
        }
    }
        return false;
}

void gpio_callback_handler(uint gpio, uint32_t eventmask) {
    int button_value = 1;
    queue_try_add(&events, &button_value);
}
    typedef enum {BOOT,DISPENSED,NOT_DISPENSED,EMPTY,POWER_OFF,RUNNING} state_t;
void send_Lora(){}
state_t read_state_from_eeprom(bool send_lora) {
    uint16_t buf = eeprom_read_byte(MEMORY_STATE_ADDR);
    state_t state = buf;
    if (send_lora) {
        switch(state) {
            case BOOT:
                printf("Booting: 0%02X\nsending message to lora\n", buf);
            lora_msg("AT+MSG=Booting",60000, "+MSG: Done");
            break;
            case DISPENSED:
                printf("Dispensing: 0%02X\nsending message to lora\n", buf);
            lora_msg("AT+MSG=Dispensed",60000, "+MSG: Done");
            break;
            case NOT_DISPENSED:
                printf("Failed to dispense: 0%02X\nsending message to lora\n", buf);
            lora_msg("AT+MSG=Not-dispensed",60000, "+MSG: Done");

            break;
            case EMPTY:
                printf("Dispenser is out of pills: 0%02X\nsending message to lora\n", buf);
            lora_msg("AT+MSG=Tray-is-empty",60000, "+MSG: Done");
            break;
            case POWER_OFF:
                printf("System was powered off while running: 0%02X\nsending message to lora\n", buf);
            lora_msg("AT+MSG=System-was-powered-off-while-running",60000, "+MSG: Done");
            break;
            case RUNNING:
                printf("Running: 0%02X\n", buf);
            break;
        }
    }
    return state;
}
int read_pill_count() {
    uint8_t buf = eeprom_read_byte(PILL_COUNT_ADDR);
    int count = (int) buf;
    return count;
}
int read_step_count() {
    uint8_t high = eeprom_read_byte(STEP_MEMORY_ADDR);
    uint8_t low  = eeprom_read_byte(STEP_MEMORY_ADDR + 1);
    int count = (int) (high << 8) | low;
    printf("Read steps %d from read-step-count", count);
    return ((uint16_t)high << 8) | low;
}
void write_pill_count_to_eeprom(int count) {
    uint8_t buf = (uint8_t) count;
    eeprom_write_byte(PILL_COUNT_ADDR, &buf);
}
void write_state_to_eeprom(state_t state) {
    uint8_t buf = (uint8_t) state;
    eeprom_write_byte(MEMORY_STATE_ADDR, &buf);
    read_state_from_eeprom(true);

}
void write_steps_to_eeprom(int steps) {
    uint8_t high = steps >> 8 & 0xFF;
    uint8_t low  = steps & 0xff;
    eeprom_write_byte(STEP_MEMORY_ADDR, &high);
    eeprom_write_byte(STEP_MEMORY_ADDR + 1, &low);
}
void try_joining() {
    bool joined = lora_join("AT+JOIN\n\r", 20000, "+JOIN: Done");
    if (!joined) {
        try_joining();
    }
}
int main() {

    stdio_init_all();

    // Initialize hardware
    init_motor();
    eeprom_init();
    lora_init();
    if (lora_is_rdy()) {
        printf("Lora is ready\n");
    }else {
        printf("Lora is not responding\n");
    }
    gpio_init(PIEZO_PIN);
    gpio_set_dir(PIEZO_PIN, GPIO_IN);
    gpio_pull_up(PIEZO_PIN);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(BUTTON_CALIB_PIN);
    gpio_set_dir(BUTTON_CALIB_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_CALIB_PIN);

    gpio_init(BUTTON_START_PIN);
    gpio_set_dir(BUTTON_START_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_START_PIN);

    gpio_init(BUTTON_EXTRA_PIN);
    gpio_set_dir(BUTTON_EXTRA_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_EXTRA_PIN);
    int pills_left = 7;
    int steps;
    queue_init(&events, sizeof(int), 10);
    gpio_set_irq_enabled_with_callback(PIEZO_PIN, GPIO_IRQ_EDGE_FALL,true,gpio_callback_handler);
    state_t state = read_state_from_eeprom(false);

    if (state == RUNNING) {
        //if system gets rebooted while it is running itll alert about it and reads saved informaation from eeprom
        //like how many steps are in 1 revolution and how many pills are left
        pills_left = read_pill_count();
        steps = read_step_count();
        //goes backwards and stops on 1st slot
        reverse_calibrate_wheel((7-pills_left), steps);
        printf("Pills left: %d\n", pills_left);
        printf("calibrated Steps: %d\n", steps);
        lora_command("AT+MODE=LWOTAA\n\r", 10000, "+MODE: LWOTAA");
        lora_command("AT+KEY=APPKEY, [passkey here]\n\r", 10000,"+KEY");
        lora_command("AT+CLASS=A\n\r", 10000, "+CLASS: A");
        lora_command("AT+PORT=8\n\r", 10000, "+PORT: 8");
        sleep_ms(50);
        bool done = false;
        while (!done) {
            tight_loop_contents();
            done = lora_join("AT+JOIN\n\r", 20000, "+JOIN: Done");
            printf("JOIN if true %d\n", done);
            sleep_ms(10000);
        }
        write_state_to_eeprom(POWER_OFF);
        printf("re-joining done\n");
    }else {
        //indicates device has booted
        lora_command("AT+MODE=LWOTAA\n\r", 500, "+MODE: LWOTAA");
        lora_command("AT+KEY=APPKEY, [passkey here]\n\r", 500,"+KEY");
        lora_command("AT+CLASS=A\n\r", 500, "+CLASS: A");
        lora_command("AT+PORT=8\n\r", 500, "+PORT: 8");
        bool done = false;
        while (!done) {
            tight_loop_contents();
            done = lora_join("AT+JOIN\n\r", 20000, "+JOIN: Done");
            printf("JOIN if true %d\n", done);
            sleep_ms(10000);
        }
        printf("Joining done\n");
        sleep_ms(50);
        write_state_to_eeprom(BOOT);
        // 1) Wait for calibration button press (LED blinks)
        while (gpio_get(BUTTON_CALIB_PIN)) {
            blink_led(1);
        }
        // 2) Calibrate wheel and store steps in 16 bit memory
        steps = calibrate_wheel();
        write_steps_to_eeprom(steps);

        // 3) Indicate ready state (LED solid)
        gpio_put(LED_PIN, 1);

    }
    // 4) Wait for start button press

    while (gpio_get(BUTTON_START_PIN)) {
        blink_led(1);
    }

    // 5) Dispense loop
    while (true) {
        if (pills_left == 0) {
            printf("All pills dispensed!\n");
            write_state_to_eeprom(EMPTY);
            blink_led(5);

            // require recalibratio on next cycle
            while (gpio_get(BUTTON_CALIB_PIN)) {
                blink_led(1);
            }
            pills_left = 7;


            calibrate_wheel();
            gpio_put(LED_PIN, 1);
            while (gpio_get(BUTTON_START_PIN)) {
                blink_led(1);

            }
            continue;
        }
        int value;
        while (queue_try_remove(&events, &value)){}
        run_motor(1);

        if (detect_pill()) {
            printf("Pill OK\n");
            write_state_to_eeprom(DISPENSED);
            pills_left--;
        } else {
            printf("Drop failed!\n");
            write_state_to_eeprom(NOT_DISPENSED);
            //blink_led(5);
            pills_left--;
        }
        write_pill_count_to_eeprom(pills_left);
        write_state_to_eeprom(RUNNING);
        sleep_ms(30000);
    }

    return 0;
}
