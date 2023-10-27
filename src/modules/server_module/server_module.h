#pragma once

#include <zephyr/kernel.h>

#include "../../common/location.h"

// Lock semaphore until initialized.
K_SEM_DEFINE(modem_available, 0, 1);

void server_send_van_location(struct Location location, double ts);

void server_send_ridership(double ts);

k_msgq_init(&my_msgq, my_msgq_buffer, sizeof(struct data_item_type), 10);