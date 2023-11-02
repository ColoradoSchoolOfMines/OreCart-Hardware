#pragma once

#include <zephyr/kernel.h>

struct VanInfo {
    uint8_t van_id;
    char van_id_str[3];
};

typedef struct VanInfo VanInfo;
extern VanInfo van_info;
