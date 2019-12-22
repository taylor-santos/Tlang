#include <stddef.h>
#include "builtins.h"

const char *BUILTIN_NAMES[]      = { "int", "double", "string", "bool" };
const char *BUILTIN_TYPES[]      = { "int ",
                                     "double ",
                                     "char *",
                                     "unsigned char " };
const size_t BUILTIN_COUNT       = sizeof(BUILTIN_NAMES) /
                                   sizeof(*BUILTIN_NAMES);
const Binop *BINARY_OPS[] = {
    (const Binop[]){ /* INT */
        {"=",  INT},
        {"+",  INT},
        {"-",  INT},
        {"*",  INT},
        {"/",  INT},
        {"+=", INT},
        {"-=", INT},
        {"*=", INT},
        {"/=", INT},
        {"==", BOOL},
        {"!=", BOOL},
        {"<",  BOOL},
        {"<=", BOOL},
        {">",  BOOL},
        {">=", BOOL},
        {0, 0}
    },
    (const Binop[]){ /* DOUBLE */
        {"=",  DOUBLE},
        {"+",  DOUBLE},
        {"-",  DOUBLE},
        {"*",  DOUBLE},
        {"/",  DOUBLE},
        {"+=", DOUBLE},
        {"-=", DOUBLE},
        {"*=", DOUBLE},
        {"/=", DOUBLE},
        {"==", BOOL},
        {"!=", BOOL},
        {"<",  BOOL},
        {"<=", BOOL},
        {">",  BOOL},
        {">=", BOOL},
        {0, 0}
    },
    (const Binop[]){ /* STRING */
        {"=",  STRING},
        {"+",  STRING},
        {"+=", STRING},
        {"==", BOOL},
        {"!=", BOOL},
        {"<",  BOOL},
        {"<=", BOOL},
        {">",  BOOL},
        {">=", BOOL},
        {0, 0}
    },
    (const Binop[]){ /* BOOL */
        {"=",  BOOL},
        {"==", BOOL},
        {"!=", BOOL},
        {"||",  BOOL},
        {"&&", BOOL},
        {0, 0}
    }
};

