#include "safe.h"

int asprintf(char **strp, const char *fmt, ...) {
    va_list args;
    int size;

    va_start(args, fmt);
    size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    *strp = malloc(size);
    if (*strp == NULL) {
        return -1;
    }
    va_start(args, fmt);
    return vsprintf(*strp, fmt, args);
}
