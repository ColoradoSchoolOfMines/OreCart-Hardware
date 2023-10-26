#pragma once

#include <zephyr/kernel.h>

// Lock semaphore until initialized.
K_SEM_DEFINE(modem_available, 0, 1);