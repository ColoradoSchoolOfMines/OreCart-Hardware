#pragma once

#include <zephyr/kernel.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/socket.h>

#define PRIORITY 5

#define MAX_INFLIGHT_REQUESTS 3
#define MAX_RESPONSE_LEN 128
#define PROTOCOL "HTTP/1.1"
#define STACK_SIZE 1024
#define TIMEOUT 4000

// Standard header for all requests
char *const HEADERS[] = {
    "Connection: close\r\n",
    NULL
};

struct Request {
    char* url;
    char* host;
    uint16_t port;
    enum http_method request_type;

    char* payload;
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
    struct k_work work; // Every scheduled task needs to be bound to a kernel work instance.
    
    struct SchedulerMeta* scheduler_meta;
    struct Request* request;
};

struct InFlightRequest {
    int sock;
    struct NetRequestTask* net_request_task;
    struct http_request http_request;

    uint8_t recv_buf[MAX_RESPONSE_LEN];
};

struct k_work_q net_work_queue;

K_SEM_DEFINE(http_requests_sem, MAX_INFLIGHT_REQUESTS, MAX_INFLIGHT_REQUESTS);
K_THREAD_STACK_DEFINE(net_stack_area, STACK_SIZE);

void start_scheduler();
int schedule_request(struct Request* request, struct SchedulerMeta* scheduler_meta);