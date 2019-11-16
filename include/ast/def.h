#ifndef TLANG_DEF_H
#define TLANG_DEF_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_def_data ASTDefData;
typedef struct ast_def_vtable ASTDefVTable;

struct ast_def_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    const Vector *lhs;
    const void *rhs;
};

struct ast_def_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_DEF_H
