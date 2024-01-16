#include "net_scheduler.h"

struct k_work_q net_work_queue;

K_SEM_DEFINE(http_requests_sem, MAX_INFLIGHT_REQUESTS, MAX_INFLIGHT_REQUESTS);
K_THREAD_STACK_DEFINE(net_stack_area, STACK_SIZE);

static void _work_handler(k_work* work);
static void _on_http_response(struct http_response *rsp, enum http_final_call done_signal, void* req_void);
inline static bool _should_discard_task(struct NetRequestTask* task);
inline static void _setup_http(struct NetRequestTask* task, uint8_t* recv_buf, struct http_request* http_req);
inline static std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> _gen_addrinfo(std::string& host, uint16_t port);
inline static void _reschedule_task(NetRequestTask* task);

// Standard header for all requests
const char *const HEADERS[] = {
    "Connection: close\r\n",
    NULL
};


void start_scheduler() {
    OC_LOG_INFO("Starting Network Scheduler...");
    k_work_queue_init(&net_work_queue);
    k_work_queue_start(&net_work_queue, net_stack_area,
        K_THREAD_STACK_SIZEOF(net_stack_area), PRIORITY,
        NULL
    );
}

// schedule_request takes ownership of both request and scheduler_meta (and all children)
void schedule_request(std::unique_ptr<Request> request, std::unique_ptr<SchedulerMeta> scheduler_meta) {
    OC_LOG_DEBUG("Scheduling Request");

    // Must create raw pointer here due to the C nature of Zephyr, which recommends.
    // This must therefore be cleaned up explcitly when the request is done.
    struct NetRequestTask* task = new NetRequestTask;

    task->scheduler_meta = std::move(scheduler_meta);
    task->request = std::move(request);

    // TODO: Check for errors
    k_work_init(&task->work, _work_handler);
    k_work_submit_to_queue(&net_work_queue, &task->work);
}

// Process queue items, send them over to the server.
static void _work_handler(k_work* work) {
    OC_LOG_DEBUG("Network Work Handler Called");
    k_sleep(K_SECONDS(2));

    struct NetRequestTask* task = CONTAINER_OF(work, struct NetRequestTask, work);

    if (_should_discard_task(task)) {
        OC_LOG_DEBUG("Discarding Expired Task");
        delete task;
        return;
    }

    // Semaphore limits inflight requests, this makes us wait
    // if there are still requests that haven't been handled.
    k_sem_take(&http_requests_sem, K_FOREVER);

    // Generate an In-Flight Request
    InFlightRequest* flight_req = new InFlightRequest;

    try {
        flight_req->sock = -1;
        flight_req->net_request_task = task;

        // Setup the actual kernel HTTP request
        memset(&flight_req->http_request, 0, sizeof(struct http_request));
        _setup_http(task, flight_req->recv_buf, &flight_req->http_request);

        OC_LOG_DEBUG("DNS Lookup %s %d", task->request->host.c_str(), task->request->port);

        // DNS Lookup
        auto addr = _gen_addrinfo(task->request->host, task->request->port);

        int err = connect_socket(addr->ai_family, addr->ai_addr, task->request->host.c_str(), &flight_req->sock);
        if (err) {
            OC_LOG_ERROR("connect_socket() failed, err: %d\n", err);
            throw "connect_socket() failed";
        }

        http_client_req(flight_req->sock, &flight_req->http_request, TIMEOUT, flight_req);
    } catch(...) {
        try {
            // Something failing here is real bad, means we probably have 0 connectivity.
            // Wait some amount of time for things to resolve themselves.
            k_sleep(K_MSEC(400));
            if (flight_req->sock > -1)
                close(flight_req->sock);
            _reschedule_task(task);    
        } catch(...) {
            delete task;
        }

        delete flight_req;

        // Don't block future requests.
        k_sem_give(&http_requests_sem); 
    }
}

static void _on_http_response(struct http_response *rsp, enum http_final_call done_signal, void* req_void) {
    struct InFlightRequest* req = (struct InFlightRequest*)req_void;

    bool response_done = done_signal == HTTP_DATA_FINAL;
    if (!response_done) {
        OC_LOG_INFO("Packet Received");
        return;
    }

    OC_LOG_DEBUG("Received HTTP Response");

    // If we failed, put this back on the request queue.
    if (rsp->http_status_code != 200) {
        OC_LOG_WARN("Response failed with status code: %d", rsp->http_status_code);
        _reschedule_task(req->net_request_task);
    } else {
        OC_LOG_INFO("Response success with status code: %d", rsp->http_status_code);

        delete req->net_request_task;
        req->net_request_task = NULL;
    }
    

    close(req->sock);
    delete req;
    k_sem_give(&http_requests_sem);
}

inline static bool _should_discard_task(struct NetRequestTask* task) {
    uint64_t uptime = k_uptime_get();
    return task->scheduler_meta->tries == 0 || uptime > task->scheduler_meta->expiration;
}

inline static void _setup_http(struct NetRequestTask* task, uint8_t* recv_buf, struct http_request* http_req) {
    http_req->method = task->request->request_type;
    http_req->url = task->request->url.c_str();
    http_req->host = task->request->host.c_str();
    http_req->protocol = PROTOCOL;
    http_req->payload = (const char*)task->request->payload.get();
    http_req->payload_len = task->request->payload_size;
    http_req->content_type_value = "application/octet-stream";
    http_req->header_fields = (const char **)HEADERS;
    http_req->response = _on_http_response;
    http_req->recv_buf_len = MAX_RESPONSE_LEN;
    http_req->recv_buf = recv_buf;
}

// Allocate an addrinfo object for the caller, populating the host and port through a DNS lookup.
inline static std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> _gen_addrinfo(std::string& host, uint16_t port) {
    addrinfo* res;
    addrinfo hints = {
		.ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
	};

    int err = getaddrinfo(host.c_str(), NULL, &hints, &res);
	if (err) {
		OC_LOG_ERROR("getaddrinfo() failed, err %d   %d", errno, err);
		throw "getaddrinfo() failed";
	}

    ((struct sockaddr_in *)(res)->ai_addr)->sin_port = htons(port);

    return std::unique_ptr<addrinfo, decltype(&freeaddrinfo)>(res, freeaddrinfo);
}

inline static void _reschedule_task(NetRequestTask* task) {
    // Push current item back to the queue.
    task->scheduler_meta->tries--;
    int err = k_work_submit_to_queue(&net_work_queue, &task->work);
    if (err < 0)
        throw "Could not resubmit work!";
}