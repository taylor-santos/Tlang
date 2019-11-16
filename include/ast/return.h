#ifndef TLANG_RETURN_H
#define TLANG_RETURN_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_return_data ASTReturnData;
typedef struct ast_return_vtable ASTReturnVTable;
struct ast_return_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    const ASTNode *returns;
};

struct ast_return_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_RETURN_H
