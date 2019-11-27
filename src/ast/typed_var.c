#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/typed_var.h"

static void free_typed_var(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode   *node = this;
    ASTTypedVarData *data = node->data;
    free(data->name);
    free_VarType(data->given_type);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_typed_var(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"TypedVar\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode   *node = this;
    ASTTypedVarData *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\",\n",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"name\": ");
    fprintf(out, "\"%s\",\n", data->name);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"given type\": ");
    json_VarType(data->given_type, indent, out);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int GetType_TypedVar(const ASTNode *node,
                            const Map *symbols,
                            UNUSED TypeCheckState *state,
                            VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    ASTTypedVarData *data = node->data;
    size_t len = strlen(data->name);
    if (symbols->contains(symbols, data->name, len)) {
        safe_method_call(symbols, get, data->name, len, type_ptr);
        if (typecmp(*type_ptr, data->given_type, state, symbols, NULL)) {
            return 1;
        }
        data->type = *type_ptr;
    } else {
        *type_ptr = data->given_type;
        VarType *type_copy = NULL;
        safe_function_call(copy_VarType, *type_ptr, &type_copy);
        safe_method_call(symbols, put, data->name, len, type_copy, NULL);
        data->type = *type_ptr;
    }
    VarType *type_copy = NULL;
    safe_function_call(copy_VarType, data->type, &type_copy);
    char *name = NULL;
    safe_strdup(&name, data->name);
    return 0;
}

static char *CodeGen_TypedVar(const ASTNode *node,
                              UNUSED CodegenState *state,
                              UNUSED FILE *out) {
    ASTTypedVarData *data = node->data;
    return strdup(data->name);
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
    data->loc    = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->name          = name;
    data->given_type    = type;
    data->type          = NULL;
    data->is_hold       = 0;
    vtable->free        = free_typed_var;
    vtable->json        = json_typed_var;
    vtable->get_type    = GetType_TypedVar;
    vtable->codegen     = CodeGen_TypedVar;
    return node;
}
