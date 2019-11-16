#ifndef TLANG_TUPLE_H
#define TLANG_TUPLE_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_tuple_data ASTTupleData;
typedef struct ast_tuple_vtable ASTTupleVTable;

struct ast_tuple_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    const Vector *exprs;
};

struct ast_tuple_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_TUPLE_H
