#include <zephyr/kernel.h>

#include "logging.h"

void log_custom(const char* level, const char* format, ...) {
    va_list args;
    va_start(args, format);

    printk("[%s] ", level);
    printk(format, args);

    va_end(args);
}