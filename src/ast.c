#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include "Tlang_parser.h"

#define UNUSED __attribute__ ((unused))

void free_ASTNode(const void *this) {
    const ASTNode *node = this;
    ASTNodeVTable *vtable = node->vtable;
    vtable->free(node);
}

void free_leaf(const void *this) {
    const ASTNode *node = this;
    ASTNodeData *data = node->data;
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void*)node);
}

void json_leaf(UNUSED const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Leaf Node\"");
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_LeafNode(struct YYLTYPE *loc) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    ASTNodeData *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_program_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(node->data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    vtable->free = free_leaf;
    vtable->json = json_leaf;
    return node;
}

void free_program(const void *this) {
    const ASTNode *node = this;
    ASTProgramData *data = node->data;
    data->statements->free(data->statements, free_ASTNode);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void*)node);
}


void json_vector(const Vector *vec, int indent, FILE *out) {
    char *sep = "\n";
    fprintf(out, "[");
    int size = vec->size(vec);
    if (size > 0) {
        indent++;
        for (int i = 0; i < size; i++) {
            fprintf(out, "%s", sep);
            fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
            const ASTNode *node = NULL;
            if (vec->get(vec, i, &node)) return;
            ASTNodeVTable *vtable = node->vtable;
            vtable->json(node, indent, out);
            sep = ",\n";
        }
        fprintf(out, "\n");
        indent--;
        fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    }
    fprintf(out, "]");
}

void json_program(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Program\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"statements\": ");
    const ASTNode *node = this;
    ASTProgramData *data = node->data;
    json_vector(data->statements, indent, out);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

const ASTNode *new_ProgramNode(struct YYLTYPE *loc, const Vector *statements) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_program_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_program_vtable *vtable = malloc(sizeof(*vtable));
    if (vtable == NULL) {
        free(data);
        free(node);
        return NULL;
    }
    node->vtable = vtable;
    data->loc = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    if (statements)
        data->statements = statements;
    else
        data->statements = new_Vector(0);
    vtable->free = free_program;
    vtable->json = json_program;
    return node;
}
