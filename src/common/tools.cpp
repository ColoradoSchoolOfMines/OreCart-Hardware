#include "tools.h"

std::string string_format(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);

    // Get size of output string (+1 for NULL terminator)
    int n = vsnprintf(NULL, 0, fmt, args) + 1;

    // Create the string 
    std::string str(n, '\0');
    vsnprintf((char*)str.data(), n, fmt, args);

    va_end(args);
    return str;
}