#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/if_block.h"

static void free_if_block(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode   *node = this;
    ASTIfBlockData *data = node->data;
    free(data->name);
    free_VarType(data->type);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_if_block(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"IfBlock\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode   *node = this;
    ASTIfBlockData *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\",\n",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"cond\": ");
    json_ASTNode(data->cond, indent, out);
    fprintf(out, ",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"statements\": ");
    json_vector(data->stmts, indent, out, json_ASTNode);
    if (data->elseStmts != NULL) {
        fprintf(out, ",\n");
        fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
        fprintf(out, "\"else\": ");
        json_vector(data->elseStmts, indent, out, json_ASTNode);
    }
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int GetType_IfBlock(const ASTNode *node,
                           const Map *symbols,
                           TypeCheckState *state,
                           VarType **type_ptr) {
    return 0;
}

static char *CodeGen_IfBlock(const ASTNode *node,
                             CodegenState *state,
                             FILE *out) {
    return strdup("IF BLOCK");
}

const ASTNode *new_IfBlockNode(struct YYLTYPE *loc,
                               const ASTNode *cond,
                               const Vector *stmts,
                               const Vector *elseStmts) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_if_block_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_if_block_vtable *vtable = malloc(sizeof(*vtable));
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
    data->is_hold       = 0;
    data->name          = NULL;
    data->type          = NULL;
    data->cond          = cond;
    data->stmts         = stmts;
    data->elseStmts     = elseStmts;
    vtable->free        = free_if_block;
    vtable->json        = json_if_block;
    vtable->get_type    = GetType_IfBlock;
    vtable->codegen     = CodeGen_IfBlock;
    return node;
}
