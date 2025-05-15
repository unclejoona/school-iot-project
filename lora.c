//
// Created by Joona on 08/05/2025.
//
#include "lora.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <hardware/gpio.h>
#include <hardware/structs/io_bank0.h>
#include <pico/time.h>
#include <pico/types.h>
#include "functions.h"
#define BAUDRATE 9600
#define UART_ID 1
#define TX_PIN 4
#define RX_PIN 5
static absolute_time_t timeout;
void lora_init() {
    iuart_setup(UART_ID, TX_PIN, RX_PIN, BAUDRATE);

}
bool lora_is_rdy() {
    const char send_handshake[] = "AT\r\n";
    int STRLEN = 512;
    bool done = false;
    uint8_t handshake[STRLEN];
    int pos = 0;
    for (int max = 0; max < 5 && !done; max++) {
        iuart_send(UART_ID, send_handshake);
        timeout = make_timeout_time_ms(500);
        while (!time_reached(timeout) && !done) {
            uint8_t c;
            if (iuart_read(UART_ID, &c, 1) > 0) {
                if (c == '\n') {
                    handshake[pos] = '\0';
                    done = true;
                } else {
                    handshake[pos++] = c;
                }
            }
        }
    }
    if (pos > 0 && strstr(handshake, "+AT: OK") != NULL) {
        pos = 0;
        printf("%d received: %s\n",time_us_32(),(char*)handshake);
        //c = 0;
        return true;
    }else {
        return false;
    }
}
bool lora_command(char *command, int ms_timeout,char *response) {
    //const char suffix[] = "\r\n";
    //strcat(*command, *suffix);
    //printf("%s", command);
    int STRLEN = 512;
    bool done = false;
    int pos = 0;
    uint8_t uart_response[STRLEN];
    iuart_send(UART_ID,command);
    timeout = make_timeout_time_ms(ms_timeout);
    while (!time_reached(timeout) && done==false) {
        uint8_t c;
        if (iuart_read(UART_ID, &c, 1) > 0) {
            if (c == '\n') {

                uart_response[pos] = '\0';
                done = true;
            } else {
                uart_response[pos++] = c;
            }
        }
    }

    if (pos > 0 && strstr(uart_response, response) != NULL) {
        pos = 0;
        //printf("its in %s\n", (char*)uart_response);

    }else {
        printf("Module stopped responding\n");
    }
}
bool lora_join(char *command, int ms_timeout,char *response) {
    //const char suffix[] = "\r\n";
    //strcat(*command, *suffix);
    //printf("%s", command);
    int STRLEN = 512;
    bool done = false;
    int pos = 0;
    int index = 0;
    uint8_t uart_response[STRLEN];
    iuart_send(UART_ID,command);
    timeout = make_timeout_time_ms(ms_timeout);
    while (!time_reached(timeout) && done==false) {
        uint8_t c;
        if (iuart_read(UART_ID, &c, 1) > 0) {
            if (c == '\n' && strstr(uart_response, response) != NULL) {
                uart_response[pos] = '\0';
                //printf("STILL IN LOOP:%s\n", (char*)uart_response);
                done = true;
            } else if (c == '\n') {
                index++;
                uart_response[pos++] = c;
            }else{
                uart_response[pos++] = c;
            }
        }
    }

    if (pos > 0 && strstr(uart_response, response) != NULL) {
        pos = 0;
        printf("its in %s\n", (char*)uart_response);
    }

    if (strstr(uart_response, "+JOIN: Join failed") != NULL) {
        printf("Join failed\n");
        done = false;
    }

    return done;
}

bool lora_msg(char *command, int ms_timeout,char *response) {
    uint8_t trash;

    //const char suffix[] = "\r\n";
    //strcat(*command, *suffix);
    //printf("%s", command);

    int STRLEN = 512;
    bool done = false;
    int pos = 0;
    int index = 0;
    uint8_t uart_response[STRLEN];
    iuart_send(UART_ID,command);
    timeout = make_timeout_time_ms(ms_timeout);
    while (!time_reached(timeout) && done==false) {
        uint8_t c;
        if (iuart_read(UART_ID, &c, 1) > 0) {
            if (c == '\n' && strstr(uart_response, response) != NULL) {
                uart_response[pos] = '\0';
                //printf("STILL IN LOOP:%s\n", (char*)uart_response);
                done = true;
            } else if (c == '\n') {
                index++;
                uart_response[pos++] = c;
            }else{
                uart_response[pos++] = c;
            }
        }
    }

    if (pos > 0 && strstr(uart_response, response) != NULL) {
        pos = 0;
        printf("its in %s\n", (char*)uart_response);

    }else if (strstr(uart_response, "Join failed") != NULL) {
        printf("Module stopped responding\n");
    }else{
        printf("Module stopped responding\n");
    }
    return done;
}