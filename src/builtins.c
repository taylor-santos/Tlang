#include <stddef.h>

const char   *BUILTIN_NAMES[]    = { "int", "double", "string", "bool" };
const char   *BUILTIN_TYPES[]    = { "int ",
                                     "double ",
                                     "char *",
                                     "unsigned char " };
const char   *BINARIES[]         = { "+", "-", "*", "/" };
const char   *BOOL_BINARIES[]    = { "||", "&&" };
const size_t STRING_BINARY_COUNT = 1;
const size_t BUILTIN_COUNT       = sizeof(BUILTIN_NAMES) /
                                   sizeof(*BUILTIN_NAMES);
const size_t BINARY_COUNT        = sizeof(BINARIES) / sizeof(*BINARIES);
const size_t BOOL_BINARY_COUNT   = sizeof(BOOL_BINARIES) /
                                   sizeof(*BOOL_BINARIES);
