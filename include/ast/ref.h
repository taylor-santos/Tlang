#ifndef TLANG_REF_H
#define TLANG_REF_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_ref_data ASTRefData;
typedef struct ast_ref_vtable ASTRefVTable;

struct ast_ref_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    const ASTNode *expr;
};

struct ast_ref_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_REF_H
