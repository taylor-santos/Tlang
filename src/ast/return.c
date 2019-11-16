#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/return.h"
#include "ast/r_expr.h"
#include "codegen.h"
#include "ast/statement.h"


static void free_return(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode *node = this;
    ASTReturnData *data = node->data;
    if (data->returns != NULL) {
        free_ASTNode((void *) data->returns);
    }
    free_VarType(data->type);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_return(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Return\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTReturnData *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\"",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
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

static int GetType_Return(const ASTNode *node,
                          const Map *symbols,
                          TypeCheckState *state,
                          VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    ASTReturnData *data = node->data;
    const ASTNode *ret_expr = data->returns;
    VarType *ret_type = NULL;
    if (ret_expr != NULL) {
        ASTRExprVTable *ret_vtable = ret_expr->vtable;
        if (ret_vtable->get_type(ret_expr, symbols, state, &ret_type)) {
            //TODO: Handle failed to infer type of returned value
            fprintf(stderr, "error: failed to infer type of returned value\n");
            return 1;
        } else {
            safe_function_call(copy_VarType, ret_type, &data->type);
            *type_ptr = ret_type;
        }
    } else {
        data->type = NULL;
        *type_ptr = NULL;
    }
    if (state->curr_ret_type == NULL) {
        state->curr_ret_type = *type_ptr;
    } else if (typecmp(*type_ptr, state->curr_ret_type)) {
        //TODO: Handle incompatible return statement
        fprintf(stderr, "error: incompatible return type\n");
        return 1;
    }
    return 0;
}

static char *CodeGen_Return(const ASTNode *node,
                            CodegenState *state,
                            FILE *out) {
    const ASTReturnData *data = node->data;
    char *ret = strdup("return");
    if (data->returns != NULL) {
        const ASTNode *ret_node = data->returns;
        ASTStatementVTable *ret_vtable = ret_node->vtable;
        char *code = ret_vtable->codegen(ret_node, state, out);
        append_string(&ret, " %s", code);
        free(code);
    }
    return ret;
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
    data->loc    = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->returns    = value;
    data->type       = NULL;
    data->name       = NULL;
    data->is_hold    = 0;
    vtable->free     = free_return;
    vtable->json     = json_return;
    vtable->get_type = GetType_Return;
    vtable->codegen  = CodeGen_Return;
    return node;
}
