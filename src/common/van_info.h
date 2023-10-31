#pragma once
#include <zephyr/kernel.h>

typedef struct VanInfo VanInfo;

struct VanInfo {
    uint8_t van_id;
    char van_id_str[3];
} van_info;