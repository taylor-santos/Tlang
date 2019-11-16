#ifndef TLANG_L_EXPR_H
#define TLANG_L_EXPR_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_l_expr_data ASTLExprData;
typedef struct ast_l_expr_vtable ASTLExprVTable;
struct ast_l_expr_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
};

struct ast_l_expr_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_L_EXPR_H
