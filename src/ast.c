#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include "Tlang_parser.h"
#include "safe.h"

static void json_vector(const Vector *vec,
                        int indent,
                        FILE *out,
                        void(*element_json)(const void*, int, FILE*));
static void json_VarType(const void *this, int indent, FILE *out);
static void json_NamedArg(const void *this, int indent, FILE *out);
static void json_FuncType(const void *this, int indent, FILE *out);

void free_ASTNode(void *this) {
    // Call the vtable->free() function on any subtype of an AST node
    const ASTNode *node = this;
    ASTNodeVTable *vtable = node->vtable;
    vtable->free(node);
}

static void json_ASTNode(const void *this, int indent, FILE *out) {
    const ASTNode *node = this;
    ASTNodeVTable *vtable = node->vtable;
    vtable->json(node, indent, out);
}

static void free_leaf(const void *this) {
    const ASTNode *node = this;
    ASTNodeData *data = node->data;
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void*)node);
}

static void json_leaf(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Leaf Node\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTNodeData *data = node->data;
    fprintf(out, "\"loc\": \"%d:%d-%d:%d\"", data->loc->first_line, 
            data->loc->first_column, data->loc->last_line,
        data->loc->last_column);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_LeafNode(struct YYLTYPE *loc) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    ASTNodeData *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    ASTNodeVTable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(node->data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    vtable->free = free_leaf;
    vtable->json = json_leaf;
    return node;
}

static void free_program(const void *this) {
    const ASTNode *node = this;
    ASTProgramData *data = node->data;
    data->statements->free(data->statements, free_ASTNode);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void*)node);
}

static void json_program(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Program\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTProgramData *data = node->data;
    fprintf(out, "\"loc\": \"%d:%d-%d:%d\",\n", data->loc->first_line, 
            data->loc->first_column, data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"statements\": ");
    json_vector(data->statements, indent, out, json_ASTNode);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_ProgramNode(struct YYLTYPE *loc, const Vector *statements) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_program_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_program_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    if (statements)
        data->statements = statements;
    else
        data->statements = new_Vector(0);
    data->symbols      = new_Map(0, 0);
    safe_function_call(add_builtins, data->symbols);
    vtable->free       = free_program;
    vtable->json       = json_program;
    vtable->type_check = TypeCheck_Program;
    return node;
}

static void free_assignment(const void *this) {
    const ASTNode *node = this;
    ASTAssignmentData *data = node->data;
    free_ASTNode((void*)data->lhs);
    free_ASTNode((void*)data->rhs);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void*)node);
}

static void json_assignment(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Assignment\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTAssignmentData *data = node->data;
    fprintf(out, "\"loc\": \"%d:%d-%d:%d\",\n", data->loc->first_line,
        data->loc->first_column, data->loc->last_line,
        data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"lhs\": ");
    node = data->lhs;
    ASTNodeVTable *vtable = node->vtable;
    vtable->json(node, indent, out);
    fprintf(out, ",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"rhs\": ");
    node = data->rhs;
    vtable = node->vtable;
    vtable->json(node, indent, out);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_AssignmentNode(struct YYLTYPE *loc, const void *lhs,
                                  const void *rhs) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_assignment_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_assignment_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->lhs = lhs;
    data->rhs = rhs;
    data->type = NULL;
    vtable->free     = free_assignment;
    vtable->json     = json_assignment;
    vtable->get_type = GetType_Assignment;
    return node;
}

static void free_return(const void *this) {
    const ASTNode *node = this;
    ASTReturnData *data = node->data;
    free_ASTNode((void*)data->returns);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void*)node);
}

static void json_return(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Return\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTReturnData *data = node->data;
    fprintf(out, "\"loc\": \"%d:%d-%d:%d\"", data->loc->first_line,
        data->loc->first_column, data->loc->last_line,
        data->loc->last_column);
    if (data->returns != NULL) {
        fprintf(out, ",\n");
        fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
        fprintf(out, "\"returns\": ");
        node = data->returns;
        ASTNodeVTable *vtable = node->vtable;
        vtable->json(node, indent, out);
    }
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_ReturnNode(struct YYLTYPE *loc, const void *value) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_return_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_return_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->returns = value;
    data->type = NULL;
    vtable->free     = free_return;
    vtable->json     = json_return;
    vtable->get_type = GetType_Return;
    return node;
}

static void free_expression(const void *this) {
    const ASTNode *node = this;
    ASTExpressionData *data = node->data;
    data->exprs->free(data->exprs, free_ASTNode);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void*)node);
}

static void json_expression(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Expression\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTExpressionData *data = node->data;
    fprintf(out, "\"loc\": \"%d:%d-%d:%d\",\n", data->loc->first_line,
        data->loc->first_column, data->loc->last_line,
        data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"exprs\": ");
    json_vector(data->exprs, indent, out, json_ASTNode);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_ExpressionNode(struct YYLTYPE *loc, const Vector *exprs) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_expression_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_expression_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->exprs = exprs;
    data->type = NULL;
    vtable->free     = free_expression;
    vtable->json     = json_expression;
    vtable->get_type = GetType_Expression;
    return node;
}

static void free_ref(const void *this) {
    const ASTNode *node = this;
    ASTRefData *data = node->data;
    free_ASTNode((void*)data->val);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void*)node);
}

static void json_ref(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Ref\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTRefData *data = node->data;
    fprintf(out, "\"loc\": \"%d:%d-%d:%d\",\n", data->loc->first_line,
        data->loc->first_column, data->loc->last_line,
        data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"value\": ");
    node = data->val;
    json_ASTNode(node, indent, out);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_RefNode(struct YYLTYPE *loc, const ASTNode *val) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_ref_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_ref_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    safe_function_call(new_RefType, &data->type);
    data->val        = val;
    vtable->free     = free_ref;
    vtable->json     = json_ref;
    vtable->get_type = GetType_Ref;
    return node;
}

static void free_paren(const void *this) {
    const ASTNode *node = this;
    ASTParenData *data = node->data;
    free_ASTNode((void*)data->val);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void*)node);
}

static void json_paren(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Paren\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTParenData *data = node->data;
    fprintf(out, "\"loc\": \"%d:%d-%d:%d\",\n", data->loc->first_line,
        data->loc->first_column, data->loc->last_line,
        data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"value\": ");
    node = data->val;
    json_ASTNode(node, indent, out);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_ParenNode(struct YYLTYPE *loc, const ASTNode *val) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_paren_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_paren_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->val        = val;
    vtable->free     = free_paren;
    vtable->json     = json_paren;
    vtable->get_type = GetType_Paren;
    return node;
}

static void free_function(const void *this) {
    const ASTNode *node = this;
    ASTFunctionData *data = node->data;
    free_FuncType(data->signature);
    data->stmts->free(data->stmts, free_ASTNode);
    data->symbols->free(data->symbols, free_VarType);
    free(data->loc);
    free(node->data);
    free(node->vtable);

    free((void*)node);
}

static void json_function(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Function\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTFunctionData *data = node->data;
    fprintf(out, "\"loc\": \"%d:%d-%d:%d\",\n", data->loc->first_line,
        data->loc->first_column, data->loc->last_line,
        data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"signature\": ");
    json_FuncType(data->signature, indent, out);
    fprintf(out, ",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"statements\": ");
    json_vector(data->stmts, indent, out, json_ASTNode);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_FunctionNode(struct YYLTYPE *loc,
    FuncType *signature,
    const Vector *stmts) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_function_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_function_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->signature = signature;
    data->stmts = stmts;
    data->symbols = new_Map(0, 0);
    safe_function_call(new_VarType_from_FuncType, signature, &data->type);
    data->type->init = 1;
    vtable->free =     free_function;
    vtable->json =     json_function;
    vtable->get_type = GetType_Function;
    return node;
}

static void free_variable(const void *this) {
    const ASTNode *node = this;
    ASTVariableData *data = node->data;
    free(data->name);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void*)node);
}

static void json_variable(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Variable\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTVariableData *data = node->data;
    fprintf(out, "\"loc\": \"%d:%d-%d:%d\",\n", data->loc->first_line,
        data->loc->first_column, data->loc->last_line,
        data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"name\": ");
    fprintf(out, "\"%s\"\n", data->name);
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_VariableNode(struct YYLTYPE *loc, char *name) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_variable_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_variable_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->name = name;
    data->type = NULL;
    vtable->free        = free_variable;
    vtable->json        = json_variable;
    vtable->get_type    = GetType_Variable;
    vtable->assign_type = AssignType_Variable;
    vtable->get_vars    = GetVars_Variable;
    return node;
}

static void free_typed_var(const void *this) {
    const ASTNode *node = this;
    ASTTypedVarData *data = node->data;
    free(data->name);
    free(data->given_type);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void*)node);
}

static void json_typed_var(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"TypedVar\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTTypedVarData *data = node->data;
    fprintf(out, "\"loc\": \"%d:%d-%d:%d\",\n", data->loc->first_line,
        data->loc->first_column, data->loc->last_line,
        data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"name\": ");
    fprintf(out, "\"%s\",\n", data->name);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"given type\": ");
    json_VarType(data->given_type, indent, out);
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_TypedVarNode(struct YYLTYPE *loc,
                                char *name,
                                VarType *type) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_typed_var_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_typed_var_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->name = name;
    data->given_type = type;
    data->type = NULL;
    vtable->free        = free_typed_var;
    vtable->json        = json_typed_var;
    vtable->get_type    = GetType_TypedVar;
    vtable->assign_type = AssignType_TypedVar;
    vtable->get_vars    = GetVars_TypedVar;
    return node;
}

static void free_int(const void *this) {
    const ASTNode *node = this;
    ASTIntData *data = node->data;
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void*)node);
}

static void json_int(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Int\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTIntData *data = node->data;
    fprintf(out, "\"loc\": \"%d:%d-%d:%d\",\n", data->loc->first_line,
        data->loc->first_column, data->loc->last_line,
        data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"value\": ");
    fprintf(out, "\"%d\"\n", data->val);
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_IntNode(struct YYLTYPE *loc, int val) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_int_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_int_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->val = val;
    data->type = NULL;
    vtable->free        = free_int;
    vtable->json        = json_int;
    vtable->get_type    = GetType_Int;
    return node;
}

static void free_double(const void *this) {
    const ASTNode *node = this;
    ASTIntData *data = node->data;
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void*)node);
}

static void json_double(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Double\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTDoubleData *data = node->data;
    fprintf(out, "\"loc\": \"%d:%d-%d:%d\",\n", data->loc->first_line,
        data->loc->first_column, data->loc->last_line,
        data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"value\": ");
    fprintf(out, "\"%f\"\n", data->val);
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_DoubleNode(struct YYLTYPE *loc, double val) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_double_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_double_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->val = val;
    data->type = NULL;
    vtable->free        = free_double;
    vtable->json        = json_double;
    vtable->get_type    = GetType_Double;
    return node;
}

static void json_vector(const Vector *vec,
                        int indent,
                        FILE *out,
                        void(*element_json)(const void*, int, FILE*)) {
    char *sep = "\n";
    fprintf(out, "[");
    int size = vec->size(vec);
    if (size > 0) {
        indent++;
        for (int i = 0; i < size; i++) {
            fprintf(out, "%s", sep);
            fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
            const void *element = NULL;
            if (vec->get(vec, i, &element)) return;
            element_json(element, indent, out);
            sep = ",\n";
        }
        fprintf(out, "\n");
        indent--;
        fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    }
    fprintf(out, "]");
}

static void json_VarType(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": ");
    const VarType *type = this;
    switch(type->type) {
        case BUILTIN:
            switch(type->builtin) {
                case INT:
                    fprintf(out, "\"int\"\n");
                    break;
                case DOUBLE:
                    fprintf(out, "\"double\"\n");
                    break;
            }
            break;
        case NONE:
            fprintf(out, "\"none\"\n");
            break;
        case FUNCTION:
            fprintf(out, "\"func\",\n");
            fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
            fprintf(out, "\"signature\": ");
            json_FuncType(type->function, indent, out);
            fprintf(out, "\n");
            break;
        case REF:
            fprintf(out, "\"ref\"\n");
            break;
    }
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static void json_NamedArg(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const NamedArg *arg = this;
    if (arg->name != NULL) {
        fprintf(out, "\"name\": ");
        fprintf(out, "\"%s\",\n", arg->name);
        fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    }
    fprintf(out, "\"type\": ");
    json_VarType(arg->type, indent, out);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static void json_FuncType(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    const FuncType *type = this;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    if (type->extension != NULL) {
        fprintf(out, "\"extends\": ");
        json_NamedArg(type->extension, indent, out);
        fprintf(out, ",\n");
        fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    }
    fprintf(out, "\"args\": ");
    json_vector(type->named_args, indent, out, json_NamedArg);
    fprintf(out, ",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"returns\": ");
    json_VarType(type->ret_type, indent, out);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}
