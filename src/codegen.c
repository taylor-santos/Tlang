#include <string.h>
#include "codegen.h"
#include "safe.h"

typedef struct {
    int indent;
    const Map *func_defs;
    int tmp_count;
} CodegenState;

static void include_headers(FILE *out) {
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <string.h>\n");
    fprintf(out, "\n");
}

static void print_indent(int n, FILE *out) {
    fprintf(out, "%*s", n * INDENT_WIDTH, "");
}

static void define_closures(FILE *out) {
    fprintf(out, "typedef struct {\n");
    print_indent(1, out);
    fprintf(out, "void *fn;\n");
    print_indent(1, out);
    fprintf(out, "void **env;\n");
    fprintf(out, "} closure;\n");
    fprintf(out, "\n");
    fprintf(out, "#define call(c, args...) ((void*(*)())((closure*)c)->"
                 "fn)(((closure*)c)->env, ##args)\n");
    fprintf(out, "#define alloc(...) memcpy(malloc(sizeof((void*[]){"
                 "__VA_ARGS__})), (void*[]){__VA_ARGS__}, sizeof((void*[]){"
                 "__VA_ARGS__}))\n");
    fprintf(out, "#define build_closure(c, fn, ...) { \\\n");
    print_indent(1, out);
    fprintf(out, "*c = malloc(sizeof(closure)); \\\n");
    print_indent(1, out);
    fprintf(out, "memcpy(*c, &(closure){ fn, alloc(*c, __VA_ARGS__) }, "
                 "sizeof(closure)); \\\n");
    fprintf(out, "}\n");
    fprintf(out, "\n");
}

static void define_builtins(FILE *out) {
    fprintf(out, "typedef struct {\n");
    print_indent(1, out);
    fprintf(out, "int value;\n");
    fprintf(out, "} class_Int;\n");
    fprintf(out, "\n");
    fprintf(out, "void *new_Int(int val) {\n");
    print_indent(1, out);
    fprintf(out, "int *ret = malloc(sizeof(int));\n");
    print_indent(1, out);
    fprintf(out, "*ret = val;\n");
    print_indent(1, out);
    fprintf(out, "return ret;\n");
    fprintf(out, "}\n");
    fprintf(out, "\n");
    fprintf(out, "void println(void **env, void *val) {\n");
    print_indent(1, out);
    fprintf(out, "printf(\"%%d\\n\", ((class_Int*)val)->value);\n");
    fprintf(out, "}\n");
    fprintf(out, "\n");
}

static int define_functions(CodegenState *state, FILE *out) {
    ASTNode **nodes = NULL;
    size_t *ptr_lengths;
    int fn_count;

    safe_method_call(state->func_defs, keys, &fn_count, &ptr_lengths, &nodes);
    fprintf(out, "/* Forward declare functions */\n");
    for (int i = 0; i < fn_count; i++) {
        ASTNode *node = nodes[i];
        ASTFunctionData *data = node->data;
        fprintf(out, "void ");
        if (data->signature->ret_type->type != NONE) {
            fprintf(out, "*");
        }
        int *index = NULL;
        safe_method_call(state->func_defs, get, node, sizeof(node), &index);
        fprintf(out, "func%d(void**", *index);
        int arg_count;
        NamedArg **args = NULL;
        safe_method_call(data->signature->named_args, array, &arg_count, &args);
        for (int j = 0; j < arg_count; j++) {
            fprintf(out, ", void*");
        }
        free(args);
        fprintf(out, ");\n");
    }
    fprintf(out, "\n");
    fprintf(out, "/* Define functions */\n");
    for (int i = 0; i < fn_count; i++) {
        ASTNode *node = nodes[i];
        ASTFunctionData *data = node->data;
        fprintf(out, "void ");
        if (data->signature->ret_type->type != NONE) {
            fprintf(out, "*");
        }
        int *index = NULL;
        safe_method_call(state->func_defs, get, node, sizeof(node), &index);
        fprintf(out, "func%d(void **env", *index);
        int arg_count;
        NamedArg **args = NULL;
        safe_method_call(data->signature->named_args, array, &arg_count, &args);
        for (int j = 0; j < arg_count; j++) {
            fprintf(out, ", void *%s", args[j]->name);
        }
        free(args);
        fprintf(out, ") {\n");
        state->indent++;
        const char **symbols = NULL;
        int symbol_count;
        size_t *lengths;
        safe_method_call(data->symbols,
                         keys,
                         &symbol_count,
                         &lengths,
                         &symbols);
        if (symbol_count > 0) {
            print_indent(state->indent, out);
        }
        char *sep = "void *";
        for (int j = 0; j < symbol_count; j++) {
            if (data->locals->contains(data->locals, symbols[j], lengths[j])) {
                continue;
            }
            fprintf(out, "%s%.*s", sep, (int)lengths[j], symbols[j]);
            sep = ", *";
            free((char*)symbols[j]);
        }
        fprintf(out, ";\n");
        free(lengths);
        free(symbols);
        safe_method_call(data->env,
                         keys,
                         &symbol_count,
                         &lengths,
                         &symbols);
        for (int j = 0; j < symbol_count; j++) {
            print_indent(1, out);
            fprintf(out,
                    "%.*s = env[%d];\n",
                    (int)lengths[j],
                    symbols[j],
                    (j + 1) % symbol_count);
            free((char*)symbols[j]);
        }
        free(lengths);
        free(symbols);
        ASTNode **stmts = NULL;
        int stmt_count;
        safe_method_call(data->stmts, array, &stmt_count, &stmts);
        for (int j = 0; j < stmt_count; j++) {
            ASTNode *stmt = stmts[j];
            ASTNodeVTable *vtable = stmt->vtable;
            char *code = vtable->codegen(stmt, state, out);
            print_indent(state->indent, out);
            fprintf(out, "%s;\n", code);
        }
        free(stmts);
        state->indent--;
        fprintf(out, "}\n");
        fprintf(out, "\n");
        free(nodes[i]);
    }
    free(nodes);
    free(ptr_lengths);
    return 0;
}

int define_main(ASTProgramData *data, CodegenState *state, FILE *out) {
    fprintf(out, "int main(int argc, char *argv[]) {\n");
    state->indent++;
    const char **symbols = NULL;
    int symbol_count;
    size_t *lengths;
    safe_method_call(data->symbols, keys, &symbol_count, &lengths, &symbols);
    for (int i = 0; i < symbol_count; i++) {
        print_indent(state->indent, out);
        fprintf(out, "void *%.*s;\n", (int)lengths[i], symbols[i]);
        free((char*)symbols[i]);
    }
    free(lengths);
    free(symbols);

    print_indent(state->indent, out);
    fprintf(out, "build_closure(&var_println, println);\n");

    ASTNode **stmts = NULL;
    int stmt_count;
    safe_method_call(data->statements, array, &stmt_count, &stmts);
    for (int i = 0; i < stmt_count; i++) {
        ASTNode *stmt = stmts[i];
        ASTStatementVTable *vtable = stmt->vtable;
        char *code = vtable->codegen(stmt, state, out);
        print_indent(1, out);
        fprintf(out, "%s;\n", code);
        free(code);
    }
    free(stmts);
    state->indent--;
    fprintf(out, "}\n");
    fprintf(out, "\n");
    return 0;
}

void int_printer(const void *x) {
    printf("%d", *(int*)x);
}

char *CodeGen_Program(const void *this, UNUSED void *state, FILE *out) {
    const ASTNode *node = this;
    ASTProgramData *data = node->data;

    print_Map(data->func_defs, int_printer);

    CodegenState *codegen_state = malloc(sizeof(*codegen_state));
    if (codegen_state == NULL) {
        print_ICE("unable to allocate memory");
    }
    codegen_state->indent = 0;
    codegen_state->tmp_count = 0;
    codegen_state->func_defs = data->func_defs;

    include_headers(out);
    define_closures(out);
    define_builtins(out);
    define_functions(codegen_state, out);
    define_main(data, codegen_state, out);
    free(codegen_state);
    return 0;
}

char *CodeGen_Assignment(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    ASTAssignmentData *data = node->data;
    const ASTNode *lhs = data->lhs;
    const ASTNode *rhs = data->rhs;
    ASTNodeVTable *vtable = lhs->vtable;
    char *lhs_code = vtable->codegen(lhs, state, out);
    vtable = rhs->vtable;
    char *rhs_code = vtable->codegen(rhs, state, out);
    char *ret = NULL;
    safe_asprintf(&ret, "%s = %s", lhs_code, rhs_code);
    free(lhs_code);
    free(rhs_code);
    return ret;
}

char *CodeGen_Return(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    const ASTReturnData *data = node->data;
    char *ret = strdup("return");
    if (data->returns != NULL) {
        const ASTNode *ret_node = data->returns;
        ASTStatementVTable *ret_vtable = ret_node->vtable;
        char *str = ret_vtable->codegen(ret_node, state, out);
        append_string(&ret, " %s", str);
    }
    return ret;
}

char *CodeGen_Expression(const void *this, void *state, FILE *out) {
    CodegenState *cg_state = state;
    const ASTNode *node = this;
    ASTExpressionData *data = node->data;
    Expression *expr = data->expr_node;
    const ASTNode *expr_node = expr->node;
    ASTStatementVTable *expr_vtable = expr_node->vtable;
    switch(expr->type) {
        case EXPR_FUNC:;
            char *func_code = expr_vtable->codegen(expr_node, state, out);
            ASTNode **args = NULL;
            int arg_count;
            safe_method_call(expr->args, array, &arg_count, &args);
            char **arg_codes = malloc(sizeof(*arg_codes) * arg_count);
            for (int i = 0; i < arg_count; i++) {
                ASTNode *arg = args[i];
                ASTStatementVTable *arg_vtable = arg->vtable;
                arg_codes[i] = arg_vtable->codegen(arg, state, out);
            }
            free(args);
            char *ret = NULL;
            safe_asprintf(&ret, "tmp%d", cg_state->tmp_count++);
            print_indent(cg_state->indent, out);
            fprintf(out, "void *%s = ", ret);
            fprintf(out, "call(%s", func_code);
            free(func_code);
            for (int i = 0; i < arg_count; i++) {
                fprintf(out, ", %s", arg_codes[i]);
                free(arg_codes[i]);
            }
            free(arg_codes);
            fprintf(out, ");\n");
            return ret;
        case EXPR_VALUE:;
            return expr_vtable->codegen(expr_node, state, out);
    }
    return NULL;
}

char *CodeGen_Ref(const void *this, void *state, FILE *out) {
    return strdup("REFERENCE");
}

char *CodeGen_Paren(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    ASTParenData *data = node->data;
    ASTStatementVTable *vtable = data->val->vtable;
    return vtable->codegen(data->val, state, out);
}

char *CodeGen_Function(const void *this, void *state, FILE *out) {
    CodegenState *cg_state = state;
    const ASTNode *node = this;
    ASTFunctionData *data = node->data;
    int *index = NULL;
    safe_method_call(cg_state->func_defs, get, node, sizeof(node), &index);
    char *ret = NULL;
    safe_asprintf(&ret, "tmp%d", cg_state->tmp_count++);
    print_indent(cg_state->indent, out);
    fprintf(out, "void *%s;\n", ret);
    print_indent(cg_state->indent, out);
    fprintf(out, "build_closure(&%s, func%d, ", ret, *index);
    const char **symbols = NULL;
    int symbol_count;
    size_t *lengths;
    safe_method_call(data->env,
                     keys,
                     &symbol_count,
                     &lengths,
                     &symbols);
    char *sep = "";
    for (int j = 0; j < symbol_count - 1; j++) {
        fprintf(out, "%s%.*s", sep, (int)lengths[j], symbols[j]);
        sep = ", ";
        free((void*)symbols[j]);
    }
    free((void*)symbols[symbol_count - 1]);
    free(lengths);
    free(symbols);
    fprintf(out, ");\n");
    return ret;
}

char *CodeGen_Class(const void *this, void *state, FILE *out) {
    return strdup("CLASS");
}

char *CodeGen_Int(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    ASTIntData *data = node->data;
    char *ret;
    safe_asprintf(&ret, "new_Int(%d)", data->val);
    return ret;
}

char *CodeGen_Double(const void *this, void *state, FILE *out) {
    return strdup("DOUBLE");
}

char *CodeGen_Variable(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    ASTVariableData *data = node->data;
    return strdup(data->name);
}

char *CodeGen_TypedVar(const void *this, void *state, FILE *out) {
    return strdup("TYPEDVAR");
}
