#ifndef BUILTINS_H
#define BUILTINS_H

extern const char *BUILTIN_NAMES[];
extern const char *BUILTIN_TYPES[];
extern const char *BINARIES[];
typedef enum { INT, DOUBLE, STRING } BUILTINS;
extern const size_t BUILTIN_COUNT;
extern const size_t BINARY_COUNT;

#endif//BUILTINS_H
