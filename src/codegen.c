#include "codegen.h"
#include "safe.h"

typedef struct {
    int indent;
    const Map *func_defs;
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
    fprintf(out, "closure *build_closure(void *fn, void **env) {\n");
    print_indent(1, out);
    fprintf(out, "closure *c = malloc(sizeof(*c));\n");
    print_indent(1, out);
    fprintf(out, "*c = (closure){fn, env};\n");
    print_indent(1, out);
    fprintf(out, "return c;\n");
    fprintf(out, "}\n");
    fprintf(out, "\n");
    fprintf(out, "#define call(c, args...) ((void*(*)())((closure*)c)->"
                 "fn)(((closure*)c)->env, ##args)\n");
    fprintf(out, "#define alloc(...) memcpy(malloc(sizeof((void*[]){"
                 "__VA_ARGS__})), (void*[]){__VA_ARGS__}, sizeof((void*[]){"
                 "__VA_ARGS__}))\n");
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

static int cast_function(FuncType *func, FILE *out) {
    fprintf(out, "(void ");
    if (func->ret_type->type != NONE) {
        fprintf(out, "*");
    }
    fprintf(out, "(*)(");
    int arg_count = func->named_args->size(func->named_args);
    fprintf(out, "void*");
    for (int j = 0; j < arg_count; j++) {
        fprintf(out, ", void*");
    }
    fprintf(out, "))");
    return 0;
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
        safe_method_call(data->env,
                         keys,
                         &symbol_count,
                         &lengths,
                         &symbols);
        for (int j = 0; j < symbol_count; j++) {
            print_indent(1, out);
            fprintf(out, "void *%.*s = env[%d];\n", (int)lengths[j], symbols[j], j);
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
            print_indent(state->indent, out);
            vtable->codegen(stmt, state, out);
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
    const char **symbols = NULL;
    int symbol_count;
    size_t *lengths;
    safe_method_call(data->symbols, keys, &symbol_count, &lengths, &symbols);
    for (int i = 0; i < symbol_count; i++) {
        print_indent(1, out);
        fprintf(out, "void *%.*s;\n", (int)lengths[i], symbols[i]);
        free((char*)symbols[i]);
    }
    free(lengths);
    free(symbols);

    print_indent(1, out);
    fprintf(out, "var_println = build_closure(println, NULL);\n");

    ASTNode **stmts = NULL;
    int stmt_count;
    safe_method_call(data->statements, array, &stmt_count, &stmts);
    for (int i = 0; i < stmt_count; i++) {
        ASTNode *stmt = stmts[i];
        ASTStatementVTable *vtable = stmt->vtable;
        print_indent(1, out);
        if (vtable->codegen(stmt, state, out)) {
            return 1;
        }
        fprintf(out, ";\n");
    }
    free(stmts);

    fprintf(out, "}\n");
    fprintf(out, "\n");
    return 0;
}

void int_printer(const void *x) {
    printf("%d", *(int*)x);
}

int CodeGen_Program(const void *this, UNUSED void *state, FILE *out) {
    const ASTNode *node = this;
    ASTProgramData *data = node->data;

    print_Map(data->func_defs, int_printer);

    CodegenState *codegen_state = malloc(sizeof(*codegen_state));
    if (codegen_state == NULL) {
        print_ICE("unable to allocate memory");
    }
    codegen_state->indent = 0;
    codegen_state->func_defs = data->func_defs;

    include_headers(out);
    define_closures(out);
    define_builtins(out);
    define_functions(codegen_state, out);
    define_main(data, codegen_state, out);
    free(codegen_state);
    return 0;
}

int CodeGen_Assignment(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    ASTAssignmentData *data = node->data;
    const ASTNode *lhs = data->lhs;
    ASTNodeVTable *vtable = lhs->vtable;
    if (vtable->codegen(lhs, state, out)) {
        return 1;
    }
    fprintf(out, " = ");
    const ASTNode *rhs = data->rhs;
    vtable = rhs->vtable;
    if (vtable->codegen(rhs, state, out)) {
        return 1;
    }
    return 0;
}

int CodeGen_Return(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    const ASTReturnData *data = node->data;
    fprintf(out, "return");
    if (data->returns != NULL) {
        fprintf(out, " ");
        const ASTNode *ret = data->returns;
        ASTStatementVTable *ret_vtable = ret->vtable;
        safe_function_call(ret_vtable->codegen, ret, state, out);
        fprintf(out, ";\n");
    }
    return 0;
}

int CodeGen_Expression(const void *this, void *state, FILE *out) {
    CodegenState *cg_state = state;
    const ASTNode *node = this;
    ASTExpressionData *data = node->data;
    Expression *expr = data->expr_node;
    const ASTNode *expr_node = expr->node;
    ASTStatementVTable *expr_vtable = expr_node->vtable;
    ASTStatementData *expr_data = expr_node->data;
    switch(expr->type) {
        case EXPR_FUNC:
            fprintf(out, "call(");
            safe_function_call(expr_vtable->codegen, expr_node, state, out);
            ASTNode **args = NULL;
            int arg_count;
            safe_method_call(expr->args, array, &arg_count, &args);
            for (int i = 0; i < arg_count; i++) {
                ASTNode *arg = args[i];
                ASTStatementVTable *arg_vtable = arg->vtable;
                fprintf(out, ", ");
                safe_function_call(arg_vtable->codegen, arg, state, out);
            }
            free(args);
            fprintf(out, ")");
            break;
        case EXPR_VALUE:;
            safe_function_call(expr_vtable->codegen, expr_node, state, out);
            break;
    }
    return 0;
}

int CodeGen_Ref(const void *this, void *state, FILE *out) {
    fprintf(out, "REFERENCE\n");
    return 0;
}

int CodeGen_Paren(const void *this, void *state, FILE *out) {
    ASTNode *node = this;
    ASTParenData *data = node->data;
    ASTStatementVTable *vtable = data->val->vtable;
    safe_function_call(vtable->codegen, data->val, state, out);
    return 0;
}

int CodeGen_Function(const void *this, void *state, FILE *out) {
    CodegenState *cg_state = state;
    const ASTNode *node = this;
    ASTFunctionData *data = node->data;
    int *index = NULL;
    safe_method_call(cg_state->func_defs, get, node, sizeof(node), &index);
    fprintf(out, "build_closure(func%d, alloc(", *index);
    const char **symbols = NULL;
    int symbol_count;
    size_t *lengths;
    safe_method_call(data->env,
                     keys,
                     &symbol_count,
                     &lengths,
                     &symbols);
    char *sep = "";
    for (int j = 0; j < symbol_count; j++) {
        fprintf(out, "%s%.*s", sep, (int)lengths[j], symbols[j]);
        sep = ", ";
        free((char*)symbols[j]);
    }
    free(lengths);
    free(symbols);
    fprintf(out, "))");
    return 0;
}

int CodeGen_Class(const void *this, void *state, FILE *out) {
    fprintf(out, "CLASS\n");
    return 0;
}

int CodeGen_Int(const void *this, void *state, FILE *out) {
    ASTNode *node = this;
    ASTIntData *data = node->data;
    fprintf(out, "new_Int(%d)", data->val);
    return 0;
}

int CodeGen_Double(const void *this, void *state, FILE *out) {
    fprintf(out, "DOUBLE\n");
    return 0;
}

int CodeGen_Variable(const void *this, void *state, FILE *out) {
    const ASTNode *node = this;
    ASTVariableData *data = node->data;
    fprintf(out, "%s", data->name);
    return 0;
}

int CodeGen_TypedVar(const void *this, void *state, FILE *out) {
    fprintf(out, "TYPEDVAR\n");
    return 0;
}
