#ifndef TLANG_TRAIT_H
#define TLANG_TRAIT_H

#include "codegen.h"
#include "map.h"
#include "vector.h"
#include "typechecker.h"

typedef struct ast_trait_data ASTTraitData;
typedef struct ast_trait_vtable ASTTraitVTable;

struct ast_trait_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
    const Vector *supers; // Vector<NamedType*>
    const Vector *stmts;
    const Map *fields;
};

struct ast_trait_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_TRAIT_H
