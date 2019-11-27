#include <stdio.h>
#include <string.h>
#include "safe.h"

int asprintf(char **strp, const char *fmt, ...) {
    va_list args;
    int size;

    va_start(args, fmt);
    size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    *strp = malloc(size + 1);
    if (*strp == NULL) {
        return -1;
    }
    va_start(args, fmt);
    return vsprintf(*strp, fmt, args);
}

int append_string(char **strp, const char *fmt, ...) {
    va_list args;
    size_t added_size, old_size = *strp ? strlen(*strp) : 0;

    va_start(args, fmt);
    added_size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    *strp = realloc(*strp, old_size + added_size + 1);
    if (*strp == NULL) {
        return -1;
    }
    va_start(args, fmt);
    return vsprintf(*strp + old_size, fmt, args);
}

int safe_strdup(const void *dup_ptr, const void *str) {
    if (dup_ptr == NULL) {
        return 1;
    }
    *(const char**)dup_ptr = strdup(str);
    if (*(const char**)dup_ptr == NULL) {
        return 1;
    }
    return 0;
}
