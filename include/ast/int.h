#ifndef TLANG_INT_H
#define TLANG_INT_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_int_data ASTIntData;
typedef struct ast_int_vtable ASTIntVTable;

struct ast_int_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    int val;
};

struct ast_int_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_INT_H
