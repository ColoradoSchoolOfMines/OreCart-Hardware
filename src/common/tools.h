#pragma once

// Function to allocate a formatted string using a template
char* alloc_sprintf(const char* template, ...) {
    // Use variable arguments to handle the template values
    va_list args;
    va_start(args, template);

    // Determine the size needed for the formatted string
    size_t size = vsnprintf(NULL, 0, template, args) + 1; // +1 for null terminator

    // Allocate memory for the formatted string
    char* buffer = (char*)malloc(size);

    // Format the string using vsnprintf into the allocated buffer
    vsnprintf(buffer, size, template, args);

    va_end(args);

    return buffer;
}