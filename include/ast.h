#ifndef AST_H
#define AST_H

#include <stdio.h>
#include "vector.h"

#define JSON_TAB_WIDTH 4

typedef struct ast_node        ASTNode;
typedef struct ast_node_data   ASTNodeData;
typedef struct ast_node_vtable ASTNodeVTable;

struct ast_node {
    void *data;
    void *vtable;
};

struct ast_node_data {
    struct YYLTYPE *loc;
};

struct ast_node_vtable {
    void   (*free)(const void*);
    void   (*json)(const void*, int, FILE*);
};

void free_ASTNode(const void*);

const ASTNode *new_LeafNode(struct YYLTYPE *loc);

typedef struct ast_program_data   ASTProgramData;
typedef struct ast_program_vtable ASTProgramVTable;


struct ast_program_data {
    struct YYLTYPE *loc;
    const Vector *statements;
};

struct ast_program_vtable {
    void   (*free)(const void*);
    void   (*json)(const void*, int, FILE*);
};

const ASTNode *new_ProgramNode(struct YYLTYPE *loc, const Vector *statements);


#endif//AST_H
