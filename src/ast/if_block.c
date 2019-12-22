#include <stdlib.h>
#include <string.h>
#include <ast/statement.h>
#include <ast/program.h>
#include "builtins.h"
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
                           UNUSED VarType **type_ptr) {
    ASTIfBlockData *data = node->data;
    ASTStatementVTable *cond_vtable = data->cond->vtable;
    VarType *cond_type = NULL;
    if (cond_vtable->get_type(data->cond, symbols, state, &cond_type)) {
        return 1;
    }
    if (!is_bool(cond_type)) {
        //TODO: Handle non-bool condition
        fprintf(stderr, "error: if statement condition must have boolean "
                       "type\n");
        return 1;
    }
    const Map *true_symbols = NULL;
    safe_method_call(symbols, copy, &true_symbols, copy_VarType);
    size_t num_stmts = data->stmts->size(data->stmts);
    for (size_t i = 0; i < num_stmts; i++) {
        const ASTNode *stmt = NULL;
        safe_method_call(data->stmts, get, i, &stmt);
        ASTStatementVTable *stmt_vtable = stmt->vtable;
        VarType *type = NULL;
        if (stmt_vtable->get_type(stmt, true_symbols, state, &type)) {
            return 1;
        }
    }
    if (data->elseStmts != NULL) {
        const Map *false_symbols = NULL;
        safe_method_call(symbols, copy, &false_symbols, copy_VarType);
        num_stmts = data->elseStmts->size(data->elseStmts);
        for (size_t i = 0; i < num_stmts; i++) {
            const ASTNode *stmt = NULL;
            safe_method_call(data->elseStmts, get, i, &stmt);
            ASTStatementVTable *stmt_vtable = stmt->vtable;
            VarType *type = NULL;
            if (stmt_vtable->get_type(stmt, false_symbols, state, &type)) {
                return 1;
            }
        }
        size_t num_symbols;
        size_t *symbol_lengths = NULL;
        char **symbol_names = NULL;
        safe_method_call(false_symbols,
                         keys,
                         &num_symbols,
                         &symbol_lengths,
                         &symbol_names);
        for (size_t i = 0; i < num_symbols; i++) {
            if (!symbols->contains(symbols,
                                   symbol_names[i],
                                   symbol_lengths[i])) {
                VarType *true_type = NULL;
                if (!true_symbols->get(true_symbols,
                                       symbol_names[i],
                                       symbol_lengths[i],
                                       &true_type)) {
                    VarType *false_type = NULL;
                    safe_method_call(false_symbols,
                                     get,
                                     symbol_names[i],
                                     symbol_lengths[i],
                                     &false_type);
                    VarType *intersection = NULL;
                    if (!typeIntersection(true_type,
                                          false_type,
                                          symbols,
                                          state,
                                          &intersection)) {
                        safe_method_call(symbols,
                                         put,
                                         symbol_names[i],
                                         symbol_lengths[i],
                                         intersection,
                                         NULL);
                    }
                } else { // True symbols does not contain this symbol
                    safe_method_call(data->falseSymbols,
                                     append,
                                     strndup(symbol_names[i],
                                             symbol_lengths[i]));
                }
            }
            free(symbol_names[i]);
        }
        free(symbol_lengths);
        free(symbol_names);
        safe_method_call(true_symbols,
                         keys,
                         &num_symbols,
                         &symbol_lengths,
                         &symbol_names);
        for (size_t i = 0; i < num_symbols; i++) {
            if (!symbols->contains(symbols,
                                   symbol_names[i],
                                   symbol_lengths[i]) &&
                !false_symbols->contains(false_symbols,
                                         symbol_names[i],
                                         symbol_lengths[i])) {
                safe_method_call(data->trueSymbols,
                                 append,
                                 strndup(symbol_names[i],
                                         symbol_lengths[i]));
            }
            free(symbol_names[i]);
        }
        free(symbol_lengths);
        free(symbol_names);
    }
    return 0;
}

static char *CodeGen_IfBlock(const ASTNode *node,
                             CodegenState *state,
                             FILE *out) {
    ASTIfBlockData *data = node->data;
    ASTStatementVTable *cond_vtable = data->cond->vtable;
    char *cond_code = cond_vtable->codegen(data->cond, state, out);
    print_indent(state->indent, out);
    fprintf(out, "if (((class%d*)%s)->val) {\n", BOOL, cond_code);
    free(cond_code);
    state->indent++;
    size_t num_symbols = data->trueSymbols->size(data->trueSymbols);
    char *sep = "void *";
    if (num_symbols) print_indent(state->indent, out);
    for (size_t i = 0; i < num_symbols; i++) {
        char *symbol = NULL;
        safe_method_call(data->trueSymbols, get, i, &symbol);
        fprintf(out, "%svar_%s", sep, symbol);
        sep = ", *";
    }
    if (num_symbols) fprintf(out, ";\n");
    size_t num_stmts = data->stmts->size(data->stmts);
    for (size_t i = 0; i < num_stmts; i++) {
        const ASTNode *stmt = NULL;
        safe_method_call(data->stmts, get, i, &stmt);
        ASTStatementVTable *stmt_vtable = stmt->vtable;
        free(stmt_vtable->codegen(stmt, state, out));
    }
    state->indent--;
    if (data->elseStmts != NULL) {
        print_indent(state->indent, out);
        fprintf(out, "} else {\n");
        state->indent++;
        num_symbols = data->falseSymbols->size(data->falseSymbols);
        sep = "void *";
        if (num_symbols) print_indent(state->indent, out);
        for (size_t i = 0; i < num_symbols; i++) {
            char *symbol = NULL;
            safe_method_call(data->falseSymbols, get, i, &symbol);
            fprintf(out, "%svar_%s", sep, symbol);
            sep = ", *";
        }
        if (num_symbols) fprintf(out, ";\n");
        num_stmts = data->elseStmts->size(data->elseStmts);
        for (size_t i = 0; i < num_stmts; i++) {
            const ASTNode *stmt = NULL;
            safe_method_call(data->elseStmts, get, i, &stmt);
            ASTStatementVTable *stmt_vtable = stmt->vtable;
            free(stmt_vtable->codegen(stmt, state, out));
        }
        state->indent--;
    }
    print_indent(state->indent, out);
    fprintf(out, "}\n");

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
    data->trueSymbols   = new_Vector(0);
    data->falseSymbols  = new_Vector(0);
    vtable->free        = free_if_block;
    vtable->json        = json_if_block;
    vtable->get_type    = GetType_IfBlock;
    vtable->codegen     = CodeGen_IfBlock;
    return node;
}
