//
// Created by Joona on 08/05/2025.
//

#ifndef LORA_H
#define LORA_H
#include <stdbool.h>

void lora_init();
bool lora_is_rdy();
bool lora_command(char *command, int ms_timeout,char *response);
bool lora_join(char *command, int ms_timeout,char *response);
bool lora_msg(char *command, int ms_timeout,char *response);


#endif //LORA_H
