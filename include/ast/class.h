#ifndef TLANG_CLASS_H
#define TLANG_CLASS_H

#include "codegen.h"
#include "map.h"
#include "vector.h"
#include "typechecker.h"

typedef struct ast_class_data ASTClassData;
typedef struct ast_class_vtable ASTClassVTable;

struct ast_class_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    const Vector *inheritance;
    const Vector *stmts;
    const Map *symbols;
    const Map *env;     // Map<char*, int>
    const Map *fields;
};

struct ast_class_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_CLASS_H
