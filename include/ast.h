#ifndef AST_H
#define AST_H

#include <stdio.h>
#include "vector.h"
#include "typechecker.h"

#define JSON_TAB_WIDTH 2

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
void free_ASTNode(void*);


/* Program Node < AST Node */
typedef struct ast_program_data   ASTProgramData;
typedef struct ast_program_vtable ASTProgramVTable;
struct ast_program_data {
    struct YYLTYPE *loc;
    const Vector   *statements; // Vector<ASTNode*>
    const Map      *symbols;
};
struct ast_program_vtable {
    void (*free)      (const void*);
    void (*json)      (const void*, int, FILE*);
    int  (*type_check)(const void*);
};
const ASTNode *new_ProgramNode(struct YYLTYPE *loc, const Vector *statements);


/* Leaf Node < AST Node*/
const ASTNode *new_LeafNode(struct YYLTYPE *loc);


/* Statement Node < AST Node */
typedef struct ast_statement_data   ASTStatementData;
typedef struct ast_statement_vtable ASTStatementVTable;
struct ast_statement_data {
    struct YYLTYPE *loc;
    VarType        *type;
};
struct ast_statement_vtable {
    void (*free)    (const void*);
    void (*json)    (const void*, int, FILE*);
    int  (*get_type)(const void*, const Map*, VarType**);
};


/* RExpr Node < Statement Node */
typedef struct ast_r_expr_data   ASTRExprData;
typedef struct ast_r_expr_vtable ASTRExprVTable;
struct ast_r_expr_data {
    struct YYLTYPE *loc;
    VarType        *type;
};
struct ast_r_expr_vtable {
    void (*free)    (const void*);
    void (*json)    (const void*, int, FILE*);
    int  (*get_type)(const void*, const Map*, VarType**);
};


/* Assignment Node < Statement Node */
typedef struct ast_assignment_data   ASTAssignmentData;
typedef struct ast_assignment_vtable ASTAssignmentVTable;
struct ast_assignment_data {
    struct YYLTYPE *loc;
    const void     *lhs;
    const void     *rhs;
    VarType        *type;
};
struct ast_assignment_vtable {
    void (*free)    (const void*);
    void (*json)    (const void*, int, FILE*);
    int  (*get_type)(const void*, const Map*, VarType**);
};
const ASTNode *new_AssignmentNode(struct YYLTYPE *loc,
                                  const void *lhs,
                                  const void *rhs);


/* Return Node < Statement Node */
typedef struct ast_return_data   ASTReturnData;
typedef struct ast_return_vtable ASTReturnVTable;
struct ast_return_data {
    struct YYLTYPE *loc;
    VarType        *type;
    const void     *returns;
};
struct ast_return_vtable {
    void (*free)    (const void*);
    void (*json)    (const void*, int, FILE*);
    int  (*get_type)(const void*, const Map*, VarType**);
};
const ASTNode *new_ReturnNode(struct YYLTYPE *loc, const void *value);


/* Expression Node < AST Node */
typedef struct ast_expression_data   ASTExpressionData;
typedef struct ast_expression_vtable ASTExpressionVTable;
struct ast_expression_data {
    struct YYLTYPE *loc;
    VarType        *type;
    const Vector   *exprs;
};
struct ast_expression_vtable {
    void (*free)    (const void*);
    void (*json)    (const void*, int, FILE*);
    int  (*get_type)(const void*, const Map*, VarType**);
};
const ASTNode *new_ExpressionNode(struct YYLTYPE *loc, const Vector *exprs);


/* Ref Node < Statement Node */
typedef struct ast_ref_data   ASTRefData;
typedef struct ast_ref_vtable ASTRefVTable;
struct ast_ref_data {
    struct YYLTYPE *loc;
    VarType        *type;
    const ASTNode  *val;
};
struct ast_ref_vtable {
    void (*free)      (const void*);
    void (*json)      (const void*, int, FILE*);
    int  (*get_type)(const void*, const Map*, VarType**);
};
const ASTNode *new_RefNode(struct YYLTYPE *loc, const ASTNode *val);


/* Paren Node < Statement Node */
typedef struct ast_paren_data   ASTParenData;
typedef struct ast_paren_vtable ASTParenVTable;
struct ast_paren_data {
    struct YYLTYPE *loc;
    VarType        *type;
    const ASTNode  *val;
};
struct ast_paren_vtable {
    void (*free)      (const void*);
    void (*json)      (const void*, int, FILE*);
    int  (*get_type)(const void*, const Map*, VarType**);
};
const ASTNode *new_ParenNode(struct YYLTYPE *loc, const ASTNode *val);


/* LExpr Node < RExpr Node */
typedef struct ast_l_expr_data   ASTLExprData;
typedef struct ast_l_expr_vtable ASTLExprVTable;
struct ast_l_expr_data {
    struct YYLTYPE *loc;
    VarType        *type;
};
struct ast_l_expr_vtable {
    void (*free)       (const void*);
    void (*json)       (const void*, int, FILE*);
    int  (*get_type)   (const void*, const Map*, VarType**);
    int  (*assign_type)(const void*, VarType*, const Map*);
    int  (*get_vars)   (const void*);
};


/* Function Node < RExpr Node */
typedef struct ast_function_data   ASTFunctionData;
typedef struct ast_function_vtable ASTFunctionVTable;
struct ast_function_data {
    struct YYLTYPE *loc;
    VarType        *type;
    FuncType       *signature;
    const Vector   *stmts; // Vector<ASTNode*>
    const Map      *symbols;
};
struct ast_function_vtable {
    void (*free)    (const void*);
    void (*json)    (const void*, int, FILE*);
    int  (*get_type)(const void*, const Map*, VarType**);
};
const ASTNode *new_FunctionNode(struct YYLTYPE *loc,
                                FuncType *signature,
                                const Vector *stmts);


/* Int Node < LExpr Node */
typedef struct ast_int_data   ASTIntData;
typedef struct ast_int_vtable ASTIntVTable;
struct ast_int_data {
    struct YYLTYPE *loc;
    VarType        *type;
    int            val;
};
struct ast_int_vtable {
    void (*free)       (const void*);
    void (*json)       (const void*, int, FILE*);
    int  (*get_type)   (const void*, const Map*, VarType**);
    int  (*assign_type)(const void*, VarType*, const Map*);
};
const ASTNode *new_IntNode(struct YYLTYPE *loc, int val);


/* Double Node < LExpr Node */
typedef struct ast_double_data   ASTDoubleData;
typedef struct ast_double_vtable ASTDoubleVTable;
struct ast_double_data {
    struct YYLTYPE *loc;
    VarType        *type;
    double         val;
};
struct ast_double_vtable {
    void (*free)       (const void*);
    void (*json)       (const void*, int, FILE*);
    int  (*get_type)   (const void*, const Map*, VarType**);
    int  (*assign_type)(const void*, VarType*, const Map*);
};
const ASTNode *new_DoubleNode(struct YYLTYPE *loc, double val);


/* Variable Node < LExpr Node */
typedef struct ast_variable_data   ASTVariableData;
typedef struct ast_variable_vtable ASTVariableVTable;
struct ast_variable_data {
    struct YYLTYPE *loc;
    VarType        *type;
    char           *name;
};
struct ast_variable_vtable {
    void (*free)       (const void*);
    void (*json)       (const void*, int, FILE*);
    int  (*get_type)   (const void*, const Map*, VarType**);
    int  (*assign_type)(const void*, VarType*, const Map*);
    int  (*get_vars)   (const void*);
};
const ASTNode *new_VariableNode(struct YYLTYPE *loc, char *name);


/* Typed Variable Node < Variable Node */
typedef struct ast_typed_var_data   ASTTypedVarData;
typedef struct ast_typed_var_vtable ASTTypedVarVTable;
struct ast_typed_var_data {
    struct YYLTYPE *loc;
    VarType        *type;
    char           *name;
    VarType        *given_type;
};
struct ast_typed_var_vtable {
    void (*free)       (const void*);
    void (*json)       (const void*, int, FILE*);
    int  (*get_type)   (const void*, const Map*, VarType**);
    int  (*assign_type)(const void*, VarType*, const Map*);
    int  (*get_vars)   (const void*);
};
const ASTNode *new_TypedVarNode(struct YYLTYPE *loc, char *name, VarType *type);

#endif//AST_H
