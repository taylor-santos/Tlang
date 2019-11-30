#include <stddef.h>

const char   *BUILTIN_NAMES[] = { "int", "double", "string" };
const char   *BUILTIN_TYPES[] = { "int ", "double ", "char *" };
const char   *BINARIES[]      = { "=", "+", "-", "*", "/" };
const size_t BUILTIN_COUNT    = sizeof(BUILTIN_NAMES) / sizeof(*BUILTIN_NAMES);
const size_t BINARY_COUNT     = sizeof(BINARIES) / sizeof(*BINARIES);
