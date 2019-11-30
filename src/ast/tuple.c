#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/tuple.h"
#include "codegen.h"
#include "ast/expression.h"
#include "ast/statement.h"

static void free_tuple(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode     *node = this;
    ASTTupleData *data = node->data;
    data->exprs->free(data->exprs, free_ASTNode);
    free_VarType(data->type);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_tuple(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Tuple\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode     *node = this;
    ASTTupleData *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\",\n",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"exprs\": ");
    json_vector(data->exprs, indent, out, json_ASTNode);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int GetType_Tuple(const ASTNode *node,
                         const Map *symbols,
                         TypeCheckState *state,
                         VarType **type_ptr) {
    ASTTupleData *data;

    if (type_ptr == NULL) {
        return 1;
    }
    data = node->data;
    size_t size = data->exprs->size(data->exprs);
    const Vector *types = new_Vector(size);
    for (size_t i = 0; i < size; i++) {
        const ASTNode *expr = NULL;
        safe_method_call(data->exprs, get, i, &expr);
        ASTExpressionVTable *vtable = expr->vtable;
        VarType *expr_type = NULL;
        if (vtable->get_type(expr, symbols, state, &expr_type)) {
            return 1;
        }
        VarType *type_copy = NULL;
        safe_function_call(copy_VarType, expr_type, &type_copy);
        safe_method_call(types, append, type_copy);
    }
    new_TupleType(&data->type, types);
    *type_ptr = data->type;
    return 0;
}

static char *CodeGen_Tuple(const ASTNode *node,
                           CodegenState *state,
                           FILE *out) {
    ASTTupleData *data = node->data;
    size_t size = data->exprs->size(data->exprs);
    char **exprs = malloc(sizeof(*exprs) * size);
    for (size_t i = 0; i < size; i++) {
        const ASTNode *expr_node = NULL;
        safe_method_call(data->exprs, get, i, &expr_node);
        ASTStatementVTable *vtable = expr_node->vtable;
        exprs[i] = vtable->codegen(expr_node, state, out);
    }
    char *ret = NULL;
    CodegenState *cg_state = state;
    asprintf(&ret, "tmp%d", cg_state->tmp_count++);
    print_indent(cg_state->indent, out);
    fprintf(out, "void **%s;\n", ret);
    print_indent(cg_state->indent, out);
    fprintf(out, "build_tuple(%s", ret);
    for (size_t i = 0; i < size; i++) {
        fprintf(out, ", %s", exprs[i]);
    }
    fprintf(out, ")\n");
    return ret;
}

const ASTNode *new_TupleNode(struct YYLTYPE *loc, const Vector *exprs) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_tuple_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_tuple_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc    = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->exprs      = exprs;
    data->type       = NULL;
    data->name       = NULL;
    data->is_hold    = 0;
    vtable->free     = free_tuple;
    vtable->json     = json_tuple;
    vtable->get_type = GetType_Tuple;
    vtable->codegen  = CodeGen_Tuple;
    return node;
}
