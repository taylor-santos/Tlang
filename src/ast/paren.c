#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/paren.h"
#include "ast/statement.h"

static void free_paren(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode *node = this;
    ASTParenData  *data = node->data;
    free_ASTNode((void *) data->val);
    free_VarType(data->type);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_paren(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Paren\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTParenData  *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\",\n",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"value\": ");
    node = data->val;
    json_ASTNode(node, indent, out);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int GetType_Paren(const ASTNode *node,
                         UNUSED const Map *symbols,
                         UNUSED TypeCheckState *state,
                         VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    ASTParenData *data = node->data;
    safe_function_call(new_ParenType, type_ptr);
    data->type = *type_ptr;
    return 0;
}

static char *CodeGen_Paren(const ASTNode *node,
                           CodegenState *state,
                           FILE *out) {
    ASTParenData *data = node->data;
    ASTStatementVTable *vtable = data->val->vtable;
    return vtable->codegen(data->val, state, out);
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
    data->loc    = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->type       = NULL;
    data->val        = val;
    data->name       = NULL;
    data->is_hold    = 0;
    vtable->free     = free_paren;
    vtable->json     = json_paren;
    vtable->get_type = GetType_Paren;
    vtable->codegen  = CodeGen_Paren;
    return node;
}

