#ifndef TLANG_EXPRESSION_H
#define TLANG_EXPRESSION_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_expression_data ASTExpressionData;
typedef struct ast_expression_vtable ASTExpressionVTable;

struct ast_expression_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    const Vector *exprs;
    Expression *expr_node;
};

struct ast_expression_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_EXPRESSION_H
