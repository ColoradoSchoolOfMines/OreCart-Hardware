#pragma once

#include <zephyr/kernel.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/socket.h>

// #include <memory>
extern "C" {
#include "socket.h"
}

#include "../../common/tools.h"
#include "../../common/logging.h"

#define PRIORITY 5

#define MAX_INFLIGHT_REQUESTS 3
#define MAX_RESPONSE_LEN 128
#define PROTOCOL "HTTP/1.1"
#define STACK_SIZE 1024
#define TIMEOUT 4000

struct Request {
    std::string url;
    std::string host;
    uint16_t port;
    enum http_method request_type;

    std::unique_ptr<uint8_t[]> payload;
    uint32_t payload_size;
};


// Scheduler Metadata for a task
struct SchedulerMeta {
    // Some expiration (in UNIX epoch milliseconds) after which it's no longer worth sending this request.
    // A lot of the requests here (like location) need to be processed quickly and quickly expire. We don't
    // want to be sending the location from 3 minutes ago.
    uint64_t expiration;
    
    // Number of times we'll allow this task to be rescheduled before we give up.
    uint8_t tries;

    // If the request needed to be discared because we reached maximum tries or we expired, callback here.
    void (*request_failed_cb)();
};

// Networking Task
struct NetRequestTask {
    // Every scheduled task needs to be bound to a kernel work instance.
    k_work work;
    
    std::unique_ptr<SchedulerMeta> scheduler_meta;
    std::unique_ptr<Request> request;
};

struct InFlightRequest {
    int sock;
    NetRequestTask* net_request_task;
    struct http_request http_request;

    uint8_t recv_buf[MAX_RESPONSE_LEN];
};

void start_scheduler();
void schedule_request(std::unique_ptr<Request> request, std::unique_ptr<SchedulerMeta> scheduler_meta);
