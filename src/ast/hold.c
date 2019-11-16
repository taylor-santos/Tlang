#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/hold.h"

static void free_hold(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode *node = this;
    ASTHoldData   *data = node->data;
    free_VarType(data->type);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_hold(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Hold\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTHoldData    *data = node->data;
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

static int GetType_Hold(const ASTNode *node,
                        UNUSED const Map *symbols,
                        UNUSED TypeCheckState *state,
                        VarType **type_ptr) {
    ASTHoldData *data = node->data;
    *type_ptr = data->type;
    return 0;
}

static char *CodeGen_Hold(UNUSED const ASTNode *node,
                          UNUSED CodegenState *state,
                          UNUSED FILE *out) {
    return strdup("HOLD");
}

const ASTNode *new_HoldNode(struct YYLTYPE *loc) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_hold_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_hold_vtable *vtable = malloc(sizeof(*vtable));
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
    new_HoldType(&data->type);
    if (data->type == NULL) {
        free(data->loc);
        free(node->vtable);
        free(data);
        free(node);
        return NULL;
    }
    data->name       = NULL;
    data->is_hold    = 1;
    vtable->free     = free_hold;
    vtable->json     = json_hold;
    vtable->get_type = GetType_Hold;
    vtable->codegen  = CodeGen_Hold;
    return node;
}
