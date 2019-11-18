#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/function.h"
#include "ast/statement.h"
#include "ast/program.h"
#include "codegen.h"

static void free_function(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode   *node = this;
    ASTFunctionData *data = node->data;
    // data->type's function field points to data->signature. Freeing data->type
    // also frees data->signature.
    free_VarType(data->type);
    data->stmts->free(data->stmts, free_ASTNode);
    if (data->symbols) data->symbols->free(data->symbols, free_VarType);
    if (data->env) data->env->free(data->env, free_VarType);
    if (data->args) data->args->free(data->args, NULL);
    free(data->loc);
    free(node->data);
    free(node->vtable);

    free((void *) node);
}

static void json_function(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Function\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode   *node = this;
    ASTFunctionData *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\",\n",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"signature\": ");
    json_FuncType(data->type->function, indent, out);
    fprintf(out, ",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"statements\": ");
    json_vector(data->stmts, indent, out, json_ASTNode);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int GetType_Function(const ASTNode *node,
                            const Map *symbols,
                            TypeCheckState *state,
                            VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    ASTFunctionData *data = node->data;
    safe_method_call(symbols, copy, &data->symbols, copy_VarType);
    safe_method_call(data->symbols, copy, &data->env, copy_VarType);
    VarType *self_type = NULL;
    safe_function_call(copy_VarType, data->type, &self_type);
    VarType *prev_self = NULL;
    char *self_name = "var_self";
    safe_method_call(data->symbols,
                     put,
                     self_name,
                     strlen(self_name),
                     self_type,
                     &prev_self);
    if (prev_self != NULL) {
        free_VarType(prev_self);
    }
    FuncType *signature = data->type->function;
    NamedType **args = NULL;
    size_t num_args;
    safe_method_call(signature->named_args, array, &num_args, &args);
    data->args = new_Map(0, 0);
    for (size_t i = 0; i < num_args; i++) {
        VarType *type_copy = NULL;
        if (args[i]->type->type == REFERENCE) {
            safe_function_call(copy_VarType,
                               args[i]->type->sub_type,
                               &type_copy);
        } else {
            safe_function_call(copy_VarType, args[i]->type, &type_copy);
        }
        size_t arg_len = strlen(args[i]->name);
        VarType *prev_type = NULL;
        safe_method_call(data->symbols,
                         put,
                         args[i]->name,
                         arg_len,
                         type_copy,
                         &prev_type);
        if (prev_type != NULL) {
            //TODO: Handle argument type conflict with outer scope variable
            fprintf(stderr, "warning: possible type conflict\n");
            free_VarType(prev_type);
        }
        safe_method_call(data->args, put, args[i]->name, arg_len, NULL, NULL);
    }
    free(args);
    size_t stmt_count = data->stmts->size(data->stmts);
    VarType *prev_ret_type = state->curr_ret_type;
    state->curr_ret_type = NULL;
    for (size_t i = 0; i < stmt_count; i++) {
        ASTNode *stmt = NULL;
        safe_method_call(data->stmts, get, i, &stmt);
        ASTStatementVTable *vtable = stmt->vtable;
        VarType *type = NULL;
        if (vtable->get_type(stmt,
                             data->symbols,
                             state,
                             &type)) {
            return 1;
        }
    }
    if (state->curr_ret_type == NULL) {
        if (data->type->function->ret_type != NULL) {
            //TODO: Handle not all code paths return a value
            fprintf(stderr, "error: not all code paths return a value\n");
            return 1;
        }
    } else {
        if (data->type->function->ret_type != NULL) {
            if (typecmp(data->type->function->ret_type,
                        state->curr_ret_type,
                        state,
                        NULL)) {
                //TODO: Handle incompatible return type
                fprintf(stderr, "error: returned value has incompatible type "
                                "to function signature\n");
                return 1;
            }
        } else {
            safe_function_call(copy_VarType, state->curr_ret_type,
                               &data->type->function->ret_type);
        }
    }
    state->curr_ret_type = prev_ret_type;
    *type_ptr = data->type;
    const ASTNode *program_ast = state->program_node;
    ASTProgramData *program_data = program_ast->data;
    int *n = malloc(sizeof(int));
    if (n == NULL) {
        print_ICE("unable to allocate memory");
    }
    *n = program_data->func_defs->size(program_data->func_defs);
    safe_method_call(program_data->func_defs, put, node, sizeof(node), n, NULL);
    return 0;
}

static char *CodeGen_Function(const ASTNode *node,
                              CodegenState *state,
                              FILE *out) {
    ASTFunctionData *data = node->data;
    int *index = NULL;
    safe_method_call(state->func_defs, get, node, sizeof(node), &index);
    char *ret = NULL;
    safe_asprintf(&ret, "tmp%d", state->tmp_count++);
    print_indent(state->indent, out);
    fprintf(out, "void *%s;\n", ret);
    print_indent(state->indent, out);
    fprintf(out, "build_closure(%s, func%d", ret, *index);
    const char **symbols = NULL;
    size_t *lengths, symbol_count;
    safe_method_call(data->env, keys, &symbol_count, &lengths, &symbols);
    for (size_t j = 0; j < symbol_count; j++) {
        VarType *var_type = NULL;
        safe_method_call(data->symbols, get, symbols[j], lengths[j], &var_type);
        fprintf(out, ", %.*s", (int) lengths[j], symbols[j]);
        free((void *) symbols[j]);
    }
    free(lengths);
    free(symbols);
    fprintf(out, ")\n");
    return ret;
}

const ASTNode *new_FunctionNode(struct YYLTYPE *loc,
                                VarType *signature,
                                const Vector *stmts) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_function_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_function_vtable *vtable = malloc(sizeof(*vtable));
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
    data->stmts      = stmts;
    data->symbols    = NULL;
    data->env        = NULL;
    data->args       = NULL;
    data->type       = signature;
    data->name       = NULL;
    data->is_hold    = 0;
    vtable->free     = free_function;
    vtable->json     = json_function;
    vtable->get_type = GetType_Function;
    vtable->codegen  = CodeGen_Function;
    return node;
}
