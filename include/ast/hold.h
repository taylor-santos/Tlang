#ifndef TLANG_HOLD_H
#define TLANG_HOLD_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_hold_data ASTHoldData;
typedef struct ast_hold_vtable ASTHoldVTable;

struct ast_hold_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
};

struct ast_hold_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_HOLD_H
