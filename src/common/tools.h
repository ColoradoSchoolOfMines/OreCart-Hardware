#pragma once
#include <string>
#include <memory>

#include <cstdarg>

#define UNIX_DELAY_MS(x) k_uptime_get() + x

template<typename D, typename B>
std::unique_ptr<D> static_cast_ptr(std::unique_ptr<B>& base) {
    return std::unique_ptr<D>(static_cast<D*>(base.release()));
}

std::string string_format(const char* fmt, ...);