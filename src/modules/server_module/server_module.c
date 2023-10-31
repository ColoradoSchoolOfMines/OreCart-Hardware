#include <zephyr/kernel.h>
#include <zephyr/net/http/client.h>

#include "server_module.h"
#include "../../common/van_info.h"

#define MAX_INFLIGHT_REQUESTS 3
#define PAYLOAD_MAX_SIZE 32
#define MAX_URL_LEN 32
#define MAX_RESP_LEN 128

#define _STACK_SIZE 256
#define _PRIORITY 5


K_THREAD_STACK_DEFINE(server_stack_area, _STACK_SIZE);
K_SEM_DEFINE(http_requests_sem, MAX_INFLIGHT_REQUESTS, MAX_INFLIGHT_REQUESTS);

struct k_work_q server_work_q;

typedef struct OreCartRequest OreCartRequest;
struct OreCartRequest {
    struct k_work work;
    struct http_request http_req;

    char payload[PAYLOAD_MAX_SIZE]; // Fixed-size payloads for now.
    char url[MAX_URL_LEN];

    char resp_buffer[MAX_RESP_LEN];
};

OreCartRequest* inflight_requests[MAX_INFLIGHT_REQUESTS]; // All buffers available by default


static void on_http_response(struct http_response *rsp,
                        enum http_final_call final_data,
                        void *user_data) {
    uint8_t inflight_index = (uint8_t)user_data;
    free(inflight_requests[inflight_index]);
    inflight_requests[inflight_index] = NULL;
    k_sem_give(&http_requests_sem);
}


// Process queue items, send them over to the server.
static void server_work_handler(struct k_work* work) {
    // Semaphore limits inflight requests, this makes us wait
    // if there are still requests that haven't been handled.
    k_sem_take(&http_requests_sem, K_FOREVER); // TODO: Add some sane timeout.

    struct OreCartRequest *request = CONTAINER_OF(
        work, struct OreCartRequest, work
    );

    // Pick first available buffer (TODO: THIS IS NOT THREAD SAFE YET!!!!)
    uint8_t i = 0;
    while (inflight_requests[i] != NULL) {
        i++;
        if (i > MAX_INFLIGHT_REQUESTS - 1) return; // Something bad happened, should never be true.
    }

    inflight_requests[i] = request;

    struct http_request* http_req = &request->http_req;

    http_req->response = on_http_response;
    http_req->recv_buf_len = sizeof(MAX_RESP_LEN);
    http_req->recv_buf = request->resp_buffer;

    // int ret = http_client_req(sock, &request->http_req, 5000, i);
}

void server_module_init() {
    k_work_queue_init(&server_work_q);

    k_work_queue_start(&server_work_q, server_stack_area,
        K_THREAD_STACK_SIZEOF(server_stack_area), _PRIORITY,
        NULL
    );
}

bool server_send_van_location(VanInfo* vanInfo, struct Location location, uint64_t ts) {
    // This will need to stay allocated until the work is finished (ie: HTTP response received)
    OreCartRequest* request = malloc(sizeof(OreCartRequest));

    // Handle memory allocation failure
    if (request == NULL) {
        return false;
    }

    struct VanLocationPayload {
        uint8_t van_id;
        double lat;
        double lon;
        uint64_t ts;
    } payload;

    payload.van_id = vanInfo->van_id;
    payload.lat = location.lat;
    payload.lon = location.lon;
    payload.ts = ts;

    memcpy(&request->payload, &payload, sizeof(struct VanLocationPayload));
    
    // Construct the route url, we need to append the van id at the end
    strcpy(request->url, SEND_VAN_LOCATION_ROUTE);
    strcpy(request->url, vanInfo->van_id_str);

    struct http_request* http_req = &request->http_req;
    http_req->method = HTTP_POST;
    http_req->url = request->url;
    http_req->host = SERVER_HOST;
    http_req->protocol = PROTOCOL;
    http_req->payload_len = sizeof(struct VanLocationPayload);
    http_req->payload = request->payload;

    k_work_submit_to_queue(&server_work_q, &request->work);
}

