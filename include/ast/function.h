#ifndef TLANG_FUNCTION_H
#define TLANG_FUNCTION_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_function_data ASTFunctionData;
typedef struct ast_function_vtable ASTFunctionVTable;

struct ast_function_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    const Vector *stmts;   // Vector<ASTNode*>
    const Map *symbols;    // Map<char*, VarType*>
    const Map *env;        // Map<char*, int>
    const Map *args;       // Map<char*, int>
};

struct ast_function_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_FUNCTION_H
