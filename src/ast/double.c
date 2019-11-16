#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/double.h"
#include "builtins.h"
#include "codegen.h"

static void free_double(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode *node = this;
    ASTDoubleData *data = node->data;
    free_VarType(data->type);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_double(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Double\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTDoubleData *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\",\n",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"value\": ");
    fprintf(out, "\"%f\"\n", data->val);
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int GetType_Double(UNUSED const ASTNode *node,
                          UNUSED const Map *symbols,
                          UNUSED TypeCheckState *state,
                          VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    ASTDoubleData *data = node->data;
    *type_ptr = data->type;
    return 0;
}

static char *CodeGen_Double(UNUSED const ASTNode *node,
                            UNUSED CodegenState *state,
                            UNUSED FILE *out) {
    ASTDoubleData *data = node->data;
    char *ret;
    safe_asprintf(&ret, "tmp%d", state->tmp_count++);
    print_indent(state->indent, out);
    fprintf(out, "void *%s = new_class%d(%f);\n", ret, DOUBLE, data->val);
    return ret;
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
    data->loc    = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    safe_function_call(new_ObjectType, &data->type);
    data->type->object->classID = DOUBLE;
    data->val                   = val;
    data->name                  = NULL;
    data->is_hold               = 0;
    vtable->free                = free_double;
    vtable->json                = json_double;
    vtable->get_type            = GetType_Double;
    vtable->codegen             = CodeGen_Double;
    return node;
}
