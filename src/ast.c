#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include "Tlang_parser.h"

#define UNUSED __attribute__ ((unused))

static void json_vector(const Vector *vec, int indent, FILE *out);

void free_ASTNode(const void *this) {
    // Call the vtable->free() function on any subtype of an AST node
    const ASTNode *node = this;
    ASTNodeVTable *vtable = node->vtable;
    vtable->free(node);
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
    json_vector(data->statements, indent, out);
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
    data->symbols = new_Map(0, 0);
    vtable->free = free_program;
    vtable->json = json_program;
    vtable->type_check = TypeCheck_Program;
    return node;
}

static void free_assignment(const void *this) {
    const ASTNode *node = this;
    ASTAssignmentData *data = node->data;
    free_ASTNode(data->lhs);
    free_ASTNode(data->rhs);
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
    vtable->free = free_assignment;
    vtable->json = json_assignment;
    vtable->get_type = GetType_Assignment;
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
    vtable->free = free_variable;
    vtable->json = json_variable;
    vtable->get_type = GetType_Variable;
    vtable->assign_type = AssignType_Variable;
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
    fprintf(out, "\"%s\"\n", data->given_type);
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_TypedVarNode(struct YYLTYPE *loc, char *name, char *type) {
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
    vtable->free = free_typed_var;
    vtable->json = json_typed_var;
    vtable->get_type = GetType_TypedVar;
    vtable->assign_type = AssignType_TypedVar;
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
    vtable->free = free_int;
    vtable->json = json_int;
    vtable->get_type = GetType_Int;
    vtable->assign_type = AssignType_Int;
    return node;
}

static void json_vector(const Vector *vec, int indent, FILE *out) {
    char *sep = "\n";
    fprintf(out, "[");
    int size = vec->size(vec);
    if (size > 0) {
        indent++;
        for (int i = 0; i < size; i++) {
            fprintf(out, "%s", sep);
            fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
            const ASTNode *node = NULL;
            if (vec->get(vec, i, &node)) return;
            ASTNodeVTable *vtable = node->vtable;
            vtable->json(node, indent, out);
            sep = ",\n";
        }
        fprintf(out, "\n");
        indent--;
        fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    }
    fprintf(out, "]");
}
