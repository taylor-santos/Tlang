#ifndef TLANG_PAREN_H
#define TLANG_PAREN_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_paren_data ASTParenData;
typedef struct ast_paren_vtable ASTParenVTable;

struct ast_paren_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    const ASTNode *val;
};

struct ast_paren_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_PAREN_H
