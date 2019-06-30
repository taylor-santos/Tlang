#ifndef AST_H
#define AST_H

#include <stdio.h>
#include "vector.h"

#define JSON_TAB_WIDTH 4

/* AST Node */
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


/* Leaf Node */
const ASTNode *new_LeafNode(struct YYLTYPE *loc);


/* Program Node < AST Node */
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


/* Statement Node < AST Node */
typedef struct ast_statement_data   ASTStatementData;
typedef struct ast_statement_vtable ASTStatementVTable;
struct ast_statement_data {
    struct YYLTYPE *loc;
};
struct ast_statement_vtable {
    void   (*free)(const void*);
    void   (*json)(const void*, int, FILE*);
};


/* Assignment Node < Statement Node */
typedef struct ast_assignment_data   ASTAssignmentData;
typedef struct ast_assignment_vtable ASTAssignmentVTable;
struct ast_assignment_data {
    struct YYLTYPE *loc;
    const void *lhs;
    const void *rhs;
};
struct ast_assignment_vtable {
    void   (*free)(const void*);
    void   (*json)(const void*, int, FILE*);
};
const ASTNode *new_AssignmentNode(struct YYLTYPE *loc,
                                  const void *lhs,
                                  const void *rhs);

/* RExpr Node < Statement Node */
typedef struct ast_r_expr_data   ASTRExprData;
typedef struct ast_r_expr_vtable ASTRExprVTable;
struct ast_r_expr_data {
    struct YYLTYPE *loc;
};
struct ast_r_expr_vtable {
    void   (*free)(const void*);
    void   (*json)(const void*, int, FILE*);
};

/* LExpr Node < RExpr Node */
typedef struct ast_l_expr_data   ASTLExprData;
typedef struct ast_l_expr_vtable ASTLExprVTable;
struct ast_l_expr_data {
    struct YYLTYPE *loc;
};
struct ast_l_expr_vtable {
    void   (*free)(const void*);
    void   (*json)(const void*, int, FILE*);
};

/* Variable Node < LExpr Node */
typedef struct ast_variable_data   ASTVariableData;
typedef struct ast_variable_vtable ASTVariableVTable;
struct ast_variable_data {
    struct YYLTYPE *loc;
    char *name;
};
struct ast_variable_vtable {
    void   (*free)(const void*);
    void   (*json)(const void*, int, FILE*);
};
const ASTNode *new_VariableNode(struct YYLTYPE *loc, char *name);

/* Int Node < LExpr Node */
typedef struct ast_int_data   ASTIntData;
typedef struct ast_int_vtable ASTIntVTable;
struct ast_int_data {
    struct YYLTYPE *loc;
    int val;
};
struct ast_int_vtable {
    void   (*free)(const void*);
    void   (*json)(const void*, int, FILE*);
};
const ASTNode *new_IntNode(struct YYLTYPE *loc, int val);

#endif//AST_H
