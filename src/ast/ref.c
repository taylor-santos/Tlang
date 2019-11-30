#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/ref.h"
#include "ast/statement.h"
#include "codegen.h"

static void free_ref(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode *node = this;
    ASTRefData    *data = node->data;
    free_VarType(data->type);
    free_ASTNode((void *) data->expr);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_ref(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Ref\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTRefData    *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\"",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int GetType_Ref(const ASTNode *node,
                       UNUSED const Map *symbols,
                       TypeCheckState *state,
                       VarType **type_ptr) {
    ASTRefData *data;
    ASTStatementVTable *vtable;

    if (type_ptr == NULL) {
        return 1;
    }
    data = node->data;
    vtable = data->expr->vtable;
    VarType *ref_type;
    if (vtable->get_type(data->expr, symbols, state, &ref_type)) {
        return 1;
    }
    if (ref_type->type == REFERENCE) {
        //TODO: Handle reference to reference
        fprintf(stderr, "error: reference to reference not allowed\n");
        return 1;
    }
    VarType *type_copy = NULL;
    safe_function_call(copy_VarType, ref_type, &type_copy);
    safe_function_call(new_RefType, &data->type, type_copy);
    *type_ptr = data->type;
    return 0;
}

static char *CodeGen_Ref(const ASTNode *node, CodegenState *state, FILE *out) {
    ASTRefData *data = node->data;
    ASTStatementVTable *ref_vtable = data->expr->vtable;
    char *code = ref_vtable->codegen(data->expr, state, out);
    ASTStatementData *ref_data = data->expr->data;
    if (ref_data->type->is_ref) {
        return code;
    }
    char *ret;
    asprintf(&ret, "tmp%d", state->tmp_count++);
    print_indent(state->indent, out);
    fprintf(out, "void **%s = &%s;\n", ret, code);
    free(code);
    return ret;
}

const ASTNode *new_RefNode(struct YYLTYPE *loc, const ASTNode *expr) {
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
    data->loc    = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->type       = NULL;
    data->expr       = expr;
    data->name       = NULL;
    data->is_hold    = 0;
    vtable->free     = free_ref;
    vtable->json     = json_ref;
    vtable->get_type = GetType_Ref;
    vtable->codegen  = CodeGen_Ref;
    return node;
}
