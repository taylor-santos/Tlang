#ifndef TLANG_STRING_H
#define TLANG_STRING_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_string_data ASTStringData;
typedef struct ast_string_vtable ASTStringVTable;

struct ast_string_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    char *val;
};

struct ast_string_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_STRING_H
