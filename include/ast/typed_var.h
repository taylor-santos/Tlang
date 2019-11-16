#ifndef TLANG_TYPED_VAR_H
#define TLANG_TYPED_VAR_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_typed_var_data ASTTypedVarData;
typedef struct ast_typed_var_vtable ASTTypedVarVTable;
struct ast_typed_var_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    VarType *given_type;
};

struct ast_typed_var_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_TYPED_VAR_H
