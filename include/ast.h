#ifndef AST_H
#define AST_H

#define JSON_TAB_WIDTH 2

#include <stdio.h>
#include "vector.h"
#include "map.h"


typedef struct expr_node Expression;
struct expr_node {
    enum {
        EXPR_VAR, EXPR_FIELD, EXPR_CONS, EXPR_FUNC, EXPR_PAREN, EXPR_HOLD
    } expr_type;
    Expression *sub_expr;
    union {
        char *name;
        Expression *arg;
    };
    const struct ASTNode *node;
    struct VarType *type;
};

void json_ASTNode(const void *this, int indent, FILE *out);
void json_vector(const Vector *vec, int indent, FILE *out,
        void(*element_json)(const void *, int, FILE *));
void json_VarType(const void *this, int indent, FILE *out);
void json_NamedArg(const void *this, int indent, FILE *out);
void json_FuncType(const void *this, int indent, FILE *out);
void json_string(const void *this, int indent, FILE *out);

/* AST Node */
typedef struct ASTNode ASTNode;
typedef struct ASTNodeVTable ASTNodeVTable;
struct ASTNode {
    void *data;
    void *vtable;
};

struct ASTNodeVTable {
    void (*free)(const void *);
    void (*json)(const void *, int, FILE *);
    char *(*codegen)(const void *, void *, FILE *);
};

struct YYLTYPE;
struct VarType;

const ASTNode *new_BoolNode(struct YYLTYPE *loc, unsigned char val);
const ASTNode *new_ClassNode(struct YYLTYPE *loc,
                             const Vector *supers,
                             const Vector *stmts);
const ASTNode *new_DefNode(struct YYLTYPE *loc,
                           const Vector *lhs,
                           const void *rhs);
const ASTNode *new_DoubleNode(struct YYLTYPE *loc, double val);
const ASTNode *new_ExpressionNode(struct YYLTYPE *loc, const Vector *exprs);
const ASTNode *new_FunctionNode(struct YYLTYPE *loc,
                                struct VarType *signature,
                                const Vector *stmts);
const ASTNode *new_HoldNode(struct YYLTYPE *loc);
const ASTNode *new_IfBlockNode(struct YYLTYPE *loc,
                               const ASTNode *cond,
                               const Vector *stmts,
                               const Vector *elseStmts);
const ASTNode *new_IntNode(struct YYLTYPE *loc, int val);
const ASTNode *new_ParenNode(struct YYLTYPE *loc, const ASTNode *val);
const ASTNode *new_ProgramNode(struct YYLTYPE *loc, const Vector *statements);
const ASTNode *new_RefNode(struct YYLTYPE *loc, const ASTNode *expr);
const ASTNode *new_ReturnNode(struct YYLTYPE *loc, const void *value);
const ASTNode *new_StringNode(struct YYLTYPE *loc, char *value);
const ASTNode *new_TraitNode(struct YYLTYPE *loc,
                             const Vector *supers,
                             const Vector *stmts);
const ASTNode *new_TupleNode(struct YYLTYPE *loc, const Vector *exprs);
const ASTNode *new_TypedVarNode(struct YYLTYPE *loc,
                                char *name,
                                struct VarType *type);
const ASTNode *new_VariableNode(struct YYLTYPE *loc, char *name);

void free_ASTNode(void *);

#endif//AST_H
