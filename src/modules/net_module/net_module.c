#include <zephyr/kernel.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/socket.h>

#include <stdio.h>

#include "net_module.h"
#include "socket.h"
#include "../../common/van_info.h"
#include "../../common/tools.h"

static const char cert[] = {
#include "../../../res/cert/ServerPublic.pem"
};

K_SEM_DEFINE(is_modem_available, 0, 1);

void net_module_init() {
    init_nrf9160_modem();
    net_module_init();
}

bool forward_van_location(VanInfo* vanInfo, struct Location location, uint64_t ts) {
    // Order types from largest to smallest for optimal data packing.
    struct VanLocationPayload {
        double lat;
        double lon;
        uint64_t ts;
    } payload;

    payload.lat = location.lat;
    payload.lon = location.lon;
    payload.ts = ts;

    struct Request* req;
    _create_request(&req, alloc_sprintf(SEND_VAN_LOCATION_ROUTE, vanInfo->van_id),
            &payload, sizeof(struct VanLocationPayload), HTTP_POST);

    // 10 second expiration with a maximum of 10 tries
    struct SchedulerMeta* meta;
    _create_scheduler_meta(&meta, k_uptime_get() + 100, 10);

    return schedule_request(req, meta);
}

// For now, do nothing if we failed to forward a van location.
void _on_forward_van_location_fail() {}

static int _create_request(struct Request** req, char* url, uint8_t* payload, uint32_t payload_size,
        enum http_method request_type) {
    *req = malloc(sizeof(struct Request));

    (*req)->host = SERVER_HOST;
    (*req)->url = url;
    (*req)->port = SERVER_PORT;
    (*req)->payload = payload;
    (*req)->payload_size = payload_size;
    (*req)->request_type = request_type;
}

static int _create_scheduler_meta(struct SchedulerMeta** meta, uint64_t expiration, uint8_t tries) {
    *meta = malloc(sizeof(struct SchedulerMeta));

    (*meta)->expiration = expiration;
    (*meta)->tries = tries;
    (*meta)->request_failed_cb = _on_forward_van_location_fail;
}
