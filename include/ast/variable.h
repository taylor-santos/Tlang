#ifndef TLANG_VARIABLE_H
#define TLANG_VARIABLE_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_variable_data ASTVariableData;
typedef struct ast_variable_vtable ASTVariableVTable;

struct ast_variable_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
};

struct ast_variable_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_VARIABLE_H
