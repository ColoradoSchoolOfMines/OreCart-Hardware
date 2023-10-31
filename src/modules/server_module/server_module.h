#pragma once

#include <zephyr/kernel.h>

#include "../../common/location.h"

#define SERVER_HOST "<URL>"
#define PROTOCOL "HTTP"


#define SEND_VAN_LOCATION_ROUTE "/location/"
#define SEND_OCCUPANCY_ROUTE "/states/ridership/"


// Lock semaphore until initialized.
K_SEM_DEFINE(modem_available, 0, 1);

void server_module_init();

// Send the current van location to the server (blocking).
// @location   -  a lat/long position of the van
// returns success boolean
bool server_send_van_location(VanInfo* vanInfo, struct Location location, uint64_t ts);

// Send the current ridership to the server (blocking).
// @ridership_delta   -   how many people got on/off the bus, negative for off and positive for on.
// returns success boolean
bool server_send_ridership(VanInfo* vanInfo, int8_t ridership_delta, uint64_t ts);

