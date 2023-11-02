#include <zephyr/kernel.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/socket.h>

#include <stdio.h>

#include "server_module.h"
#include "../../common/van_info.h"

#define MAX_INFLIGHT_REQUESTS 3
#define PAYLOAD_MAX_SIZE 64
#define MAX_URL_LEN 32
#define MAX_RESP_LEN 128

#define _STACK_SIZE 256
#define _PRIORITY 5

#define TLS_SEC_TAG 42

K_THREAD_STACK_DEFINE(server_stack_area, _STACK_SIZE);
K_SEM_DEFINE(http_requests_sem, MAX_INFLIGHT_REQUESTS, MAX_INFLIGHT_REQUESTS);

struct k_work_q server_work_q;

struct OreCartRequest {
    struct k_work work;
    struct http_request http_req;

    char payload[PAYLOAD_MAX_SIZE]; // Fixed-size payloads for now.
    char url[MAX_URL_LEN];
    int sock;

    char resp_buffer[MAX_RESP_LEN];
};
typedef struct OreCartRequest OreCartRequest;

OreCartRequest* inflight_requests[MAX_INFLIGHT_REQUESTS]; // All buffers available by default

K_SEM_DEFINE(is_modem_available, 0, 1);

static int tls_setup(int fd)
{
	int err;
	int verify;

	/* Security tag that we have provisioned the certificate with */
	const sec_tag_t tls_sec_tag[] = {
		TLS_SEC_TAG,
	};

#if defined(CONFIG_SAMPLE_TFM_MBEDTLS)
	err = tls_credential_add(tls_sec_tag[0], TLS_CREDENTIAL_CA_CERTIFICATE, cert, sizeof(cert));
	if (err) {
		return err;
	}
#endif

	/* Set up TLS peer verification */
	enum {
		NONE = 0,
		OPTIONAL = 1,
		REQUIRED = 2,
	};

	verify = REQUIRED;

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		printk("Failed to setup peer verification, err %d\n", errno);
		return err;
	}

	/* Associate the socket with the security tag
	 * we have provisioned the certificate with.
	 */
	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag, sizeof(tls_sec_tag));
	if (err) {
		printk("Failed to setup TLS sec tag, err %d\n", errno);
		return err;
	}

	err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, SERVER_HOST, sizeof(SERVER_HOST) - 1);
	if (err) {
		printk("Failed to setup TLS hostname, err %d\n", errno);
		return err;
	}
	return 0;
}

static void on_http_response(struct http_response *rsp,
                        enum http_final_call final_data,
                        void *user_data) {

    
    uint32_t inflight_index = (uint32_t)user_data;
    close(inflight_requests[inflight_index]->sock);
    free(inflight_requests[inflight_index]);
    inflight_requests[inflight_index] = NULL;
    k_sem_give(&http_requests_sem);
}


// Process queue items, send them over to the server.
static void server_work_handler(struct k_work* work) {
    k_sleep(K_SECONDS(4));
    printk("New work handle called.\r\n");


    // Semaphore limits inflight requests, this makes us wait
    // if there are still requests that haven't been handled.
    k_sem_take(&http_requests_sem, K_FOREVER); // TODO: Add some sane timeout.

    printk("Got past semaphore.\r\n");

    struct OreCartRequest *request = CONTAINER_OF(
        work, struct OreCartRequest, work
    );

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

    int err = getaddrinfo(SERVER_HOST, SERVER_PORT, &hints, &res);
	if (err) {
		printk("getaddrinfo() failed, err %d\n", errno);
		return 0;
	}

    err = tls_setup(request->sock);
	if (err) {
        return;
    }

	err = connect(request->sock, res->ai_addr, res->ai_addrlen);
	if (err) {
		printk("connect() failed, err: %d\n", errno);
		return;
	}

    request->sock = socket(res->ai_family, SOCK_STREAM | SOCK_NATIVE_TLS, IPPROTO_TLS_1_2);
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
        return false;
    }
    printk("Allocation successful.\r\n");

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

    printk("Do Payload successful.\r\n");


    memcpy(&request->payload, &payload, sizeof(struct VanLocationPayload));

    printk("Do Memcpy successful.\r\n");
    
    // Construct the route url, we need to append the van id at the end
    request->url[0] = '\0';
    strcpy(request->url, SEND_VAN_LOCATION_ROUTE);
    strcpy(request->url, vanInfo->van_id_str);

    printk("Do request url.\r\n");

    struct http_request* http_req = &request->http_req;
    http_req->method = HTTP_POST;
    http_req->url = request->url;
    http_req->host = SERVER_HOST;
    http_req->protocol = PROTOCOL;
    http_req->payload_len = sizeof(struct VanLocationPayload);
    http_req->payload = request->payload;

    printk("HTTP REQ.\r\n");

    k_work_init( &request->work, server_work_handler);

    printk("WORK INIT.\r\n");

    int err = k_work_submit_to_queue(&server_work_q, &request->work);

    if (err) {
        printf("err result: %i\r\n", err);
    }

    printk("END\r\n");

    printk("\r\n\r\n");

    return true;
}

