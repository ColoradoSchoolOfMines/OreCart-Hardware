#include <zephyr/kernel.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/socket.h>

#include <stdio.h>

#include "server_module.h"
#include "socket.h"
#include "../../common/van_info.h"
#include "../../common/tools.h"

static const char cert[] = {
#include "../../../res/cert/ServerPublic.pem"
};

#define MAX_INFLIGHT_REQUESTS 3
#define PAYLOAD_MAX_SIZE 64
#define MAX_URL_LEN 32
#define MAX_RESP_LEN 128

#define _STACK_SIZE 256
#define _PRIORITY 5

K_THREAD_STACK_DEFINE(server_stack_area, _STACK_SIZE);
K_SEM_DEFINE(http_requests_sem, MAX_INFLIGHT_REQUESTS, MAX_INFLIGHT_REQUESTS);

struct k_work_q server_work_q;

// What should we do if a request fails (runs out of tries or expires)?
enum RequestFailedAction {
    // Drop the request (forget it forever)
    DROP,

    // Try to upload it when the OreCart is in downtime (ie: parked). This can
    // be useful for analytical data that we want still want but don't quickly
    // (ie: fine to send later).
    DOWNTIME_UPLOAD
};

struct OreCartRequest {
    struct k_work work;
    struct http_request http_req;

    char payload[PAYLOAD_MAX_SIZE];
    char url[MAX_URL_LEN];
    int sock;

    // Some expiration (in UNIX epoch milliseconds) after which it's no longer worth sending this request.
    // A lot of the requests here (like location) need to be processed quickly an quickly expire. We don't
    // want to be sending the location from 3 minutes ago. This saves on networking resources and allows 
    uint64_t expiration; 
    uint8_t tries; // Number of times we'll retry sending this request before we give up.

    enum RequestFailedAction request_failed_action;

    char resp_buffer[MAX_RESP_LEN];
};

typedef struct OreCartRequest OreCartRequest;

OreCartRequest* inflight_requests[MAX_INFLIGHT_REQUESTS]; // All buffers available by default

K_SEM_DEFINE(is_modem_available, 0, 1);

static void on_http_response(struct http_response *rsp,
                        enum http_final_call final_data,
                        void *user_data) {
    if (final_data == HTTP_DATA_FINAL) {
        // printk("\r\nresp: %d\r\n", rsp->http_status_code);
        uint32_t inflight_index = (uint32_t)user_data;
    

        // If we failed, let's try and this back to the request queue.
        if (rsp->http_status_code != 200) {
            printk("\r\nResponse failed with status code: %d\r\n", rsp->http_status_code);
            inflight_requests[inflight_index]->tries--;
            k_work_submit_to_queue(&server_work_q, &inflight_requests[inflight_index]->work);
        } else {
            printk("\r\nResponse success with status code: %d\r\n", rsp->http_status_code);

             // Some response processing (TBD).
            // for (int i=0; i<rsp->content_length; i++) {
            //     printk("%X ", rsp->recv_buf[i]);
            // }
            
            free(inflight_requests[inflight_index]);
        }

        close(inflight_requests[inflight_index]->sock);
        
        inflight_requests[inflight_index] = NULL;
        k_sem_give(&http_requests_sem);

    } else {
        printk(".");
    }
}


// Process queue items, send them over to the server.
static void server_work_handler(struct k_work* work) {
    printk("New work handle called.\r\n"); 

    struct OreCartRequest *request = CONTAINER_OF(
        work, struct OreCartRequest, work
    );

    uint64_t uptime = k_uptime_get();

    if (request->tries == 0 || uptime > request->expiration) {
        printk("Request expired, discarding...\r\n");

        free(request);
        return;
    }

    // Semaphore limits inflight requests, this makes us wait
    // if there are still requests that haven't been handled.
    k_sem_take(&http_requests_sem, K_FOREVER); // TODO: Add some sane timeout.
    

    // Pick first available buffer (TODO: THIS IS NOT THREAD SAFE YET!!!!)
    uint32_t i = 0;
    while (inflight_requests[i] != NULL) {
        i++;
        if (i > MAX_INFLIGHT_REQUESTS - 1) return; // Something bad happened, should never be true.
    }
    

    inflight_requests[i] = request;

    struct http_request* http_req = &request->http_req;

    http_req->response = on_http_response;
    http_req->recv_buf_len = sizeof(MAX_RESP_LEN);
    http_req->recv_buf = request->resp_buffer;

    struct addrinfo *res;
	struct addrinfo hints = {
		.ai_flags = AI_NUMERICSERV, /* Let getaddrinfo() set port */
		.ai_socktype = SOCK_STREAM,
	};

    int err = getaddrinfo(SERVER_HOST, NULL, &hints, &res);
	if (err) {
		printk("getaddrinfo() failed, err %d   %d\n", errno, err);
		return;
	}

    char peer_addr[INET6_ADDRSTRLEN];

    inet_ntop(res->ai_family, &((struct sockaddr_in *)(res->ai_addr))->sin_addr, peer_addr,
		  INET6_ADDRSTRLEN);

    printk("Address Resolved!!! %s\r\n", peer_addr);

    struct sockaddr_in addr4;

	// err = connect(request->sock, res->ai_addr, res->ai_addrlen);
    err = connect_socket(AF_INET, peer_addr, SERVER_PORT,
				     &request->sock, (struct sockaddr *)&addr4,
				     sizeof(addr4));

	if (err) {
		printk("connect_socket() failed, err: %d\n", errno);

        // Not creating a socket is real bad, means we probably have 0 connectivity.
        k_sleep(K_MSEC(400)); // Wait some amount of time for things to resolve themselves.

        close(request->sock);

        // Push current item back to the queue.
        inflight_requests[i] = NULL;
        request->tries--;
        k_work_submit_to_queue(&server_work_q, &request->work);
        k_sem_give(&http_requests_sem); // Don't block future requests
		return;
	}

    // request->sock = socket(res->ai_family, SOCK_STREAM | SOCK_NATIVE_TLS, IPPROTO_TLS_1_2); // TLS
    // request->sock = socket(res->ai_family, SOCK_STREAM, IPPROTO_TCP);
    int ret = http_client_req(request->sock, &request->http_req, TIMEOUT, (void*)i);
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

    printk("Sending location...\r\n");

    // Handle memory allocation failure
    if (request == NULL) {
        printk("Memory allocation failure.\r\n");
        return false;
    }
    
    // Order types from largest to smallest for data packing.
    struct VanLocationPayload {
        double lat;
        double lon;
        uint64_t ts;
    } payload;

    payload.lat = location.lat;
    payload.lon = location.lon;
    payload.ts = ts;

    memcpy(&request->payload, &payload, sizeof(struct VanLocationPayload));

    char vanIdStr[4];
    uint8_t_to_str(vanInfo->van_id, vanIdStr);

    // Construct the route url, we need to append the van id at the end
    strcpy(request->url, SEND_VAN_LOCATION_ROUTE);
    strcat(request->url, vanIdStr);
    request->expiration = k_uptime_get() + 100; // 10 second expiration
    request->tries = 10; // Try maximum 10 times.

    struct http_request* http_req = &request->http_req;
    http_req->method = HTTP_POST;
    http_req->url = request->url;
    http_req->host = SERVER_HOST;
    http_req->protocol = PROTOCOL;
    http_req->payload_len = sizeof(struct VanLocationPayload);
    http_req->payload = request->payload;

    k_work_init(&request->work, server_work_handler);
    k_work_submit_to_queue(&server_work_q, &request->work);

    return true;
}

