#ifndef TLANG_STATEMENT_H
#define TLANG_STATEMENT_H

#include "map.h"
#include "vector.h"
#include "typechecker.h"
#include "codegen.h"

typedef struct ast_statement_data ASTStatementData;
typedef struct ast_statement_vtable ASTStatementVTable;

struct ast_statement_data {
    struct YYLTYPE *loc;
    VarType *type;
    char *name;
    int is_hold;
};

struct ast_statement_vtable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const ASTNode *, CodegenState *, FILE *);
    int (*get_type)(const ASTNode *, const Map *, TypeCheckState *, VarType **);
};

#endif//TLANG_STATEMENT_H
