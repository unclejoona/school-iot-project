#include "eeprom.h"

#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define EEPROM_ADDR 0x50
#define I2C_PORT i2c0
#define I2C_SDA_PIN 16
#define I2C_SCL_PIN 17

void eeprom_init() {
    i2c_init(I2C_PORT, 100 * 1000); // 100 kHz
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
}

void eeprom_write_byte(uint16_t mem_address, uint8_t *data) {

    uint8_t buf[3] = {
        (mem_address >> 8) & 0xFF,
        mem_address & 0xFF,
        *data
    };

    i2c_write_blocking(I2C_PORT, EEPROM_ADDR, buf, 3, false);
    sleep_ms(5); // Allow EEPROM internal write to complete
}

uint8_t eeprom_read_byte(uint16_t mem_address) {
    uint8_t addr_buf[2] = {
        (mem_address >> 8) & 0xFF,
        mem_address & 0xFF
    };

    i2c_write_blocking(I2C_PORT, EEPROM_ADDR, addr_buf, 2, true);
    sleep_ms(5);
    uint8_t data = 0;
    i2c_read_blocking(I2C_PORT, EEPROM_ADDR, &data, 1, false);
    return data;
}

void eeprom_write_bytes(uint16_t mem_address, const uint8_t *data, size_t length) {
    uint8_t buf[2 + length];
    buf[0] = mem_address >> 8;
    buf[1] = mem_address & 0xFF;
    for (size_t i = 0; i < length; i++) {
        buf[2 + i] = data[i];
    }
    i2c_write_blocking(I2C_PORT, EEPROM_ADDR, buf, length + 2, false);
    sleep_ms(5);
}

void eeprom_read_bytes(uint16_t mem_address, uint8_t *data, size_t length) {
    uint8_t addr_buf[2] = {
        (mem_address >> 8) & 0xFF,
        mem_address & 0xFF
    };
    i2c_write_blocking(I2C_PORT, EEPROM_ADDR, addr_buf, 2, true);
    i2c_read_blocking(I2C_PORT, EEPROM_ADDR, data, length, false);
}

void set_led_state(ledstate *ls, uint8_t value) {
    ls->state = value;
    ls->not_state = ~value;
}

bool led_state_is_valid(const ledstate *ls) {
    return ls->state == (uint8_t)(~ls->not_state);
}

void save_led_state(uint8_t state) {
    ledstate ls;
    set_led_state(&ls, state);
    eeprom_write_bytes(EEPROM_LED_STATE_ADDR, (uint8_t *)&ls, sizeof(ledstate));
}

bool load_led_state(uint8_t *state_out) {
    ledstate ls;
    eeprom_read_bytes(EEPROM_LED_STATE_ADDR, (uint8_t *)&ls, sizeof(ledstate));
    if (led_state_is_valid(&ls)) {
        *state_out = ls.state;
        return true;
    }
    return false;
}