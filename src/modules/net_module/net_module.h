#pragma once

#include <zephyr/kernel.h>

#include "net_scheduler.h"

#include "../../common/location.h"
#include "../../common/van_info.h"

#define SERVER_HOST "wpodev.intergonic.com"
#define SERVER_PORT 8888

#define SEND_VAN_LOCATION_ROUTE "/location/%i"
#define SEND_OCCUPANCY_ROUTE "/stats/ridership/"

#define MAX_INFLIGHT_REQUESTS 3
#define PAYLOAD_MAX_SIZE 64
#define MAX_URL_LEN 32
#define MAX_RESP_LEN 128

// Lock semaphore until initialized.
extern struct k_sem is_modem_available;

void net_module_init();

// Send the current van location to the server.
// @location   -  a lat/long position of the van
// returns success boolean
bool server_send_van_location(VanInfo* vanInfo, struct Location location, uint64_t ts);

// Send the current ridership to the server.
// @ridership_delta   -   how many people got on/off the bus, negative for off and positive for on.
// returns success boolean
bool server_send_ridership(VanInfo* vanInfo, int8_t ridership_delta, uint64_t ts);