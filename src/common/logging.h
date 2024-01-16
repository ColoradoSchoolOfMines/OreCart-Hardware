#pragma once

void log_custom(const char* level, const char* format, ...);

#define OC_LOG_DEBUG(fmt, ...) log_custom("DEBUG", fmt "\n", ##__VA_ARGS__)
#define OC_LOG_INFO(fmt, ...) log_custom("INFO", fmt "\n", ##__VA_ARGS__)
#define OC_LOG_WARN(fmt, ...) log_custom("WARN", fmt "\n", ##__VA_ARGS__)
#define OC_LOG_ERROR(fmt, ...) log_custom("ERROR", fmt "\n", ##__VA_ARGS__)

