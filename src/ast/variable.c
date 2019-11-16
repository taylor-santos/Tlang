#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/variable.h"

static void free_variable(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode   *node = this;
    ASTVariableData *data = node->data;
    free(data->name);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_variable(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Variable\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode   *node = this;
    ASTVariableData *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\",\n",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"name\": ");
    fprintf(out, "\"%s\"\n", data->name);
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int GetType_Variable(const ASTNode *node,
                            const Map *symbols,
                            UNUSED TypeCheckState *state,
                            VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    ASTVariableData *data = node->data;
    size_t len = strlen(data->name);
    VarType *type = NULL;
    if (!symbols->get(symbols, data->name, len, &type) && type) {
        *type_ptr = type;
        safe_function_call(copy_VarType, type, &data->type);
        return 0;
    } else {
        //TODO: Handle use before init error
        fprintf(stderr, "error: use before init\n");
        return 1;
    }
}

static char *CodeGen_Variable(const ASTNode *node,
                              UNUSED CodegenState *state,
                              UNUSED FILE *out) {
    ASTVariableData *data = node->data;
    return strdup(data->name);
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
    data->loc    = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    data->name          = name;
    data->type          = NULL;
    data->is_hold       = 0;
    vtable->free        = free_variable;
    vtable->json        = json_variable;
    vtable->get_type    = GetType_Variable;
    vtable->codegen     = CodeGen_Variable;
    return node;
}
