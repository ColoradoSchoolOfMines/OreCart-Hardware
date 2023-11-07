#pragma once

#include <stdint.h>

void uint8_t_to_str(uint8_t num, char* buff) {
    uint8_t i = 0;
    while (num != 0) {
        buff[i++] = '0' + num % 10;
        num /= 10;
    }
    buff[i] = '\0';
}
