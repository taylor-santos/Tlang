#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/bool.h"
#include "builtins.h"
#include "codegen.h"
#include "ast/program.h"

static void free_bool(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode *node = this;
    ASTBoolData    *data = node->data;
    free_VarType(data->type);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_bool(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Bool\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTBoolData    *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\",\n",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"value\": ");
    fprintf(out, "\"%d\"\n", data->val);
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int GetType_Bool(UNUSED const ASTNode *node,
                       const Map *symbols,
                       UNUSED TypeCheckState *state,
                       VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    char *name = NULL;
    safe_asprintf(&name, "%s", BUILTIN_NAMES[BOOL]);
    VarType *class_type = NULL;
    safe_method_call(symbols, get, name, strlen(name), &class_type);
    *type_ptr = class_type->class->instance;
    free(name);
    return 0;
}

static char *CodeGen_Bool(const ASTNode *node, CodegenState *state, FILE *out) {
    ASTBoolData *data = node->data;
    char *ret;
    safe_asprintf(&ret, "tmp%d", state->tmp_count++);
    print_indent(state->indent, out);
    fprintf(out, "void *%s = call(var_bool);\n", ret);
    print_indent(state->indent, out);
    fprintf(out, "((class%d*)%s)->val = %d;\n", BOOL, ret, data->val);
    return ret;
}

const ASTNode *new_BoolNode(struct YYLTYPE *loc, unsigned char val) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_bool_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_bool_vtable *vtable = malloc(sizeof(*vtable));
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
    safe_function_call(new_ObjectType, &data->type);
    data->type->object->id_type = ID;
    data->type->object->classID = BOOL;
    data->val                   = val;
    data->name                  = NULL;
    data->is_hold               = 0;
    vtable->free                = free_bool;
    vtable->json                = json_bool;
    vtable->get_type            = GetType_Bool;
    vtable->codegen             = CodeGen_Bool;
    return node;
}
