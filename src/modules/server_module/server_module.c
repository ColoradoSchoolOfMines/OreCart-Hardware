#include "server_module.h"

int send_packet() {
    k_sem_take(&modem_available, K_FOREVER);

    

    k_sem_give(&modem_available);
}