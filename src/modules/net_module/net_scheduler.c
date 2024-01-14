#include "net_scheduler.h"

static void _net_work_handler(struct k_work* work);

void start_scheduler() {
    k_work_queue_init(&net_work_queue);
    k_work_queue_start(&net_work_queue, net_stack_area,
        K_THREAD_STACK_SIZEOF(net_stack_area), PRIORITY,
        NULL
    );
}

// schedule_request takes ownership of both request and scheduler_meta
int schedule_request(struct Request* request, struct SchedulerMeta* scheduler_meta) {
    struct NetRequestTask* task = malloc(sizeof(struct NetRequestTask));
    task->scheduler_meta = scheduler_meta;

    k_work_init(&task->work, _net_work_handler);
    int success = k_work_submit_to_queue(&net_work_queue, &task->work);

    return success;    
}

// Process queue items, send them over to the server.
static void _net_work_handler(struct k_work* work) {
    struct NetRequestTask* task = CONTAINER_OF(work, struct NetRequestTask, work);

    if (_should_discard_task(task)) {
        _free_task(task);
        return;
    }

    // Semaphore limits inflight requests, this makes us wait
    // if there are still requests that haven't been handled.
    k_sem_take(&http_requests_sem, K_FOREVER);

    struct InFlightRequest* flight_req = malloc(sizeof(struct InFlightRequest));
    memset(&flight_req->http_request, 0, sizeof(struct http_request));
    _setup_http(task, flight_req->recv_buf, &flight_req->http_request);
    flight_req->net_request_task = task;

    struct addrinfo* addr;
    int err = _gen_addrinfo(addr, task->request->host, task->request->port);
    if (err) goto clean_up;

    err = connect_socket(addr->ai_family, addr->ai_addr, &flight_req->sock);
	if (err) {
		printk("connect_socket() failed, err: %d\n", errno);

        // Not creating a socket is real bad, means we probably have 0 connectivity.
        // Wait some amount of time for things to resolve themselves.
        k_sleep(K_MSEC(400));

        close(flight_req->sock);

        // Push current item back to the queue.
        task->scheduler_meta->tries--;
        k_work_submit_to_queue(&net_work_queue, &task->work);
        k_sem_give(&http_requests_sem); // Don't block future requests
		goto clean_up;
	}

    int ret = http_client_req(&flight_req->sock, &flight_req->http_request, TIMEOUT, flight_req);

clean_up:
    freeaddrinfo(addr);
    free(flight_req);
}

static void _on_http_response(struct http_response *rsp, enum http_final_call done_signal, struct InFlightRequest* req) {
    bool response_done = done_signal == HTTP_DATA_FINAL;
    if (!response_done) {
        printk(".");
        return;
    }

    // If we failed, put this back on the request queue.
    if (rsp->http_status_code != 200) {
        printk("\r\nResponse failed with status code: %d\r\n", rsp->http_status_code);
        req->net_request_task->scheduler_meta->tries--;
        k_work_submit_to_queue(&net_work_queue, &req->net_request_task->work);
    } else {
        printk("\r\nResponse success with status code: %d\r\n", rsp->http_status_code);

        _free_task(req->net_request_task); 
        free(req);
    }

    close(req->sock);
    k_sem_give(&http_requests_sem);
}

inline static bool _should_discard_task(struct NetRequestTask* task) {
    uint64_t uptime = k_uptime_get();
    return task->scheduler_meta->tries == 0 || uptime > task->scheduler_meta->tries;
}

inline static void _free_request(struct Request* request) {
    free(request->payload);
    free(request->host);
    free(request->url);
    free(request);
}

inline static void _free_task(struct NetRequestTask* task) {
    _free_request(task->request);
    free(task->scheduler_meta);
    free(task);
}

inline static void _setup_http(struct NetRequestTask* task, uint8_t* recv_buf, struct http_request* http_req) {
    http_req->method = task->request->request_type;
    http_req->url = task->request->url;
    http_req->host = task->request->host;
    http_req->protocol = PROTOCOL;
    http_req->payload = task->request->payload;
    http_req->payload_len = task->request->payload_size;
    http_req->content_type_value = "application/json";
    http_req->header_fields = (const char **)HEADERS;
    http_req->response = _on_http_response;
    http_req->recv_buf_len = sizeof(MAX_RESPONSE_LEN);
    http_req->recv_buf = recv_buf;
}

// Allocate an addrinfo object for the caller, populating the host and port through a DNS lookup.
inline static int _gen_addrinfo(struct addrinfo* res, char* host, uint16_t port) {
    struct addrinfo hints = {
		.ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
	};

    int err = getaddrinfo(host, NULL, &hints, &res);
	if (err) {
		printk("getaddrinfo() failed, err %d   %d\n", errno, err);
		return err;
	}

    ((struct sockaddr_in *)res->ai_addr)->sin_port = htons(port);
    return 0;
}