#ifndef BUILTINS_H
#define BUILTINS_H

typedef struct {
    const char *operation;
    const int ret_type;
} Binop;

extern const Binop *BINARY_OPS[];
typedef enum { ASSIGN, PLUS, PLUSASSIGN, EQUALS, NOTEQUALS, LESS, ATMOST,
               MORE, ATLEAST } STRING_OPS;
extern const char *BUILTIN_NAMES[];
extern const char *BUILTIN_TYPES[];
typedef enum { INT, DOUBLE, STRING, BOOL } BUILTINS;
extern const size_t BUILTIN_COUNT;

#endif//BUILTINS_H
