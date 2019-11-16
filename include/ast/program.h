#ifndef TLANG_PROGRAM_H
#define TLANG_PROGRAM_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_program_data ASTProgramData;
typedef struct ast_program_vtable ASTProgramVTable;

struct ast_program_data {
    struct YYLTYPE *loc;
    const Vector *statements; // Vector<ASTNode*>
    const Map *symbols;    // Map<char*, VarType*>
    const Map *func_defs;  // Map<ASTNode*, int>
    const Vector *class_defs; // Vector<ASTNode*>
};

struct ast_program_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*type_check)(const ASTNode *);
};

#endif//TLANG_PROGRAM_H