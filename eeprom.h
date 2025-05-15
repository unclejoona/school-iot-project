#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define EEPROM_LED_STATE_ADDR (0x7FFF - sizeof(ledstate) + 1)

typedef struct {
    uint8_t state;
    uint8_t not_state;
} ledstate;

void eeprom_init();
void eeprom_write_byte(uint16_t mem_address, uint8_t *data);
uint8_t eeprom_read_byte(uint16_t mem_address);
void eeprom_write_bytes(uint16_t mem_address, const uint8_t *data, size_t length);
void eeprom_read_bytes(uint16_t mem_address, uint8_t *data, size_t length);

void set_led_state(ledstate *ls, uint8_t value);
bool led_state_is_valid(const ledstate *ls);

void save_led_state(uint8_t state);
bool load_led_state(uint8_t *state_out);

#endif // EEPROM_H
