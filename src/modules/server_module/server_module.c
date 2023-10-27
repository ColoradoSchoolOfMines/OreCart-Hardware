#include "server_module.h"

#define MAX_ROUTE_LENGTH 20

enum RequestType {
    Delete,
    Get,
    Pop,
    Post,
    Push
};

struct Request {
    struct http_request request = { 0 };
};

struct ServerMsg server_msgq_buffer[10 * sizeof(struct ServerMsg)];

k_msgq_init(&my_msgq, server_msgq_buffer, sizeof(struct ServerMsg), 10);


struct Request {
    char route[20];
    char* bytes;
};

struct Response {
    int responseCode;
    char* bytes;
};


int post(struct Request request) {
    // Add this request to the event queue
}

int get(char route[20]) {
    // Add this request to the event queue
}

struct Response waitFor(int requestId) {

}

int send_packet() {
    
}

struct Request* request_queue[20];


// mainloop runs in its own thread
int mainloop() {
    while (true) {
        k_sem_take(&modem_available, K_FOREVER);
        // Some semaphore check to block if queue empty

        struct Request request = pop(request_queue);
        char* result = make_blocking_request();

        parse_result();

        // Mark some semapore to indicate that the packet is reqdy


        k_sem_give(&modem_available);
    }
}

start_thread(mainloop);