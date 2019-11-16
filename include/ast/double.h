#ifndef TLANG_DOUBLE_H
#define TLANG_DOUBLE_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_double_data ASTDoubleData;
typedef struct ast_double_vtable ASTDoubleVTable;

struct ast_double_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    double val;
};

struct ast_double_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_DOUBLE_H
