#ifndef TLANG_IF_BLOCK_H
#define TLANG_IF_BLOCK_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_if_block_data ASTIfBlockData;
typedef struct ast_if_block_vtable ASTIfBlockVTable;
struct ast_if_block_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    const ASTNode *cond;
    const Vector *stmts;
    const Vector *elseStmts;
};

struct ast_if_block_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_IF_BLOCK_H
