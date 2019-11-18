#include <stddef.h>

const char   *BUILTIN_NAMES[] = { "int", "double", "string" };
const char   *BUILTIN_TYPES[] = { "int ", "double ", "char *" };
const size_t BUILTIN_COUNT    = sizeof(BUILTIN_NAMES) / sizeof(*BUILTIN_NAMES);
