#pragma once

#include <zephyr/kernel.h>

struct VanInfo {
    uint8_t van_id;
};

typedef struct VanInfo VanInfo;
extern VanInfo van_info;
