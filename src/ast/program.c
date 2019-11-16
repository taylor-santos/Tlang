#include <stdlib.h>
#include <string.h>
#include <codegen.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/program.h"
#include "ast/statement.h"

static void free_program(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode  *node = this;
    ASTProgramData *data = node->data;
    data->statements->free(data->statements, free_ASTNode);
    data->symbols->free(data->symbols, free_VarType);
    data->func_defs->free(data->func_defs, free);
    data->class_defs->free(data->class_defs, NULL);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_program(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Program\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode  *node = this;
    ASTProgramData *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\",\n",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"statements\": ");
    json_vector(data->statements, indent, out, json_ASTNode);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int TypeCheck_Program(const ASTNode *node) {
    ASTProgramData *data = node->data;
    const Vector *stmts = data->statements;
    TypeCheckState state;
    state.new_symbols = new_Vector(0);
    state.program_node = node;
    state.curr_ret_type = NULL;
    size_t size = 0;
    ASTNode **stmt_arr = NULL;
    safe_method_call(stmts, array, &size, &stmt_arr);
    for (size_t i = 0; i < size; i++) {
        ASTStatementVTable *vtable = stmt_arr[i]->vtable;
        VarType *type = NULL;
        if (vtable->get_type(stmt_arr[i], data->symbols, &state, &type)) {
            return 1;
        }
    }
    free(stmt_arr);
    state.new_symbols->free(state.new_symbols, free_NamedType);
    return 0;
}

static char *CodeGen_Program(UNUSED const ASTNode *node,
                             UNUSED CodegenState *state,
                             UNUSED FILE *out) {
    return 0;
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
    node->vtable     = vtable;
    data->loc        = malloc(sizeof(*loc));
    if (data->loc == NULL) {
        free(vtable);
        free(data);
        free(node);
        return NULL;
    }
    memcpy(data->loc, loc, sizeof(*loc));
    if (statements) {
        data->statements = statements;
    } else {
        data->statements = new_Vector(0);
    }
    data->symbols      = new_Map(0, 0);
    data->class_defs   = new_Vector(0);
    safe_function_call(add_builtins, data->symbols, data->class_defs);
    data->func_defs    = new_Map(0, 0);
    vtable->free       = free_program;
    vtable->json       = json_program;
    vtable->type_check = TypeCheck_Program;
    vtable->codegen    = CodeGen_Program;
    return node;
}
