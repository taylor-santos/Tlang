#ifndef BUILTINS_H
#define BUILTINS_H

extern const char *BUILTIN_NAMES[];
extern const char *BUILTIN_TYPES[];
extern const char *BINARIES[];
extern const char *BOOL_BINARIES[];
typedef enum { INT, DOUBLE, STRING, BOOL } BUILTINS;
extern const size_t BUILTIN_COUNT;
extern const size_t BINARY_COUNT;
extern const size_t STRING_BINARY_COUNT;
extern const size_t BOOL_BINARY_COUNT;

#endif//BUILTINS_H
