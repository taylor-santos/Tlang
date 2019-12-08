#ifndef TLANG_BOOL_H
#define TLANG_BOOL_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_bool_data ASTBoolData;
typedef struct ast_bool_vtable ASTBoolVTable;

struct ast_bool_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    unsigned char val;
};

struct ast_bool_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_BOOL_H
