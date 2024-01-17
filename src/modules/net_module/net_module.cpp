#include "net_scheduler.h"
#include "net_module.h"
#include "socket.h"
#include "nrf9160_setup.h"

#include "../../common/van_info.h"
#include "../../common/tools.h"

static std::unique_ptr<Request> _create_http_request(std::string url, std::unique_ptr<uint8_t []> payload,
            uint32_t payload_size, enum http_method request_type);
static std::unique_ptr<SchedulerMeta> _create_scheduler_metadata(uint64_t expiration, uint8_t max_tries);

void net_module_init() {
    init_nrf9160_modem();
    start_scheduler();
}

void forward_van_location(VanInfo& vanInfo, Location& location, uint64_t ts) {
    OC_LOG_INFO("Forwarding Van Location...\n");
    // Order types from largest to smallest for optimal data packing.
    struct VanLocationPayload {
        uint64_t ts;
        double lat;
        double lon;
    };

    VanLocationPayload* payload = new VanLocationPayload;
    payload->lat = location.lat;
    payload->lon = location.lon;
    payload->ts = ts;

    std::unique_ptr<uint8_t[]> payload_bytes((uint8_t*)payload);

    std::string url = string_format(SEND_VAN_LOCATION_ROUTE, vanInfo.van_id);

    // HTTP request-specific data 
    auto req = _create_http_request(url, std::move(payload_bytes), sizeof(VanLocationPayload), HTTP_POST);
    // Metadata used by the scheduler to determine how to best schedule this request amongst all others (and when to drop)
    auto meta = _create_scheduler_metadata(UNIX_DELAY_MS(MAX_DELAY_BEFORE_DROP_MS), MAX_TRIES);

    schedule_request(std::move(req), std::move(meta));
}

// For now, do nothing if we failed to forward a van location.
void _on_forward_van_location_fail() {}

static std::unique_ptr<Request> _create_http_request(std::string url, std::unique_ptr<uint8_t []> payload,
            uint32_t payload_size, enum http_method request_type) {
    OC_LOG_DEBUG("Creating Request");

    std::unique_ptr<Request> req(new Request);

    req->host = SERVER_HOST;
    req->url = url;
    req->port = SERVER_PORT;
    req->payload = std::move(payload);
    req->payload_size = payload_size;
    req->request_type = request_type;

    return req;
}

static std::unique_ptr<SchedulerMeta> _create_scheduler_metadata(uint64_t expiration, uint8_t max_tries) {
    OC_LOG_DEBUG("Creating Scheduler Meta");

    std::unique_ptr<SchedulerMeta> meta(new SchedulerMeta);

    meta->expiration = expiration;
    meta->tries = max_tries;
    meta->request_failed_cb = _on_forward_van_location_fail;

    return meta;
}