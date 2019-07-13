#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "codegen.h"
#include "safe.h"
#include "typechecker.h"

static CodeGenState state;

static void fprintf_code(FILE *out, const char *fmt, ...) {
    va_list args;
    safe_fprintf(out, "%*s", state.indent * INDENT_WIDTH, "");
    va_start(args, fmt);
    safe_vfprintf(out, fmt, args);
    va_end(args);
}

static int declare_type(FILE *out, VarType *type, const char *name) {
    switch(type->type) {
        case BUILTIN:
            switch(type->builtin) {
                case INT:
                    fprintf_code(out, "int %s;\n", name);
                    return 0;
                case DOUBLE:
                    fprintf_code(out, "double %s;\n", name);
                    return 0;
            }
            return 1;
    }
    return 1;
}

static int declare_locals(FILE *out, const Map *locals) {
    int size = 0;
    size_t *lengths = NULL;
    const char **keys = NULL;
    safe_method_call(locals, keys, &size, &lengths, &keys);
    if (size > 0) {
        fprintf_code(out, "typedef struct locals%d locals%d;\n",
            state.locals_count, state.locals_count);
        fprintf_code(out, "struct locals%d {\n", state.locals_count);
        state.indent++;
        for (int i = 0; i < size; i++) {
            VarType *type = NULL;
            safe_method_call(locals, get, keys[i], lengths[i], &type);
            VarType *prev_type =
            safe_method_call(state.declared_vars, put, keys[i], lengths[i],
            char *name = malloc(lengths[i] + 1);
            if (name == NULL) {
                return 0;
            }
            memcpy(name, keys[i], lengths[i]);
            name[lengths[i]] = 0;
            declare_type(out, type, name);
            free(name);
            free((void*)keys[i]);
        }
        state.indent--;
        safe_fprintf(out, "};\n");
        state.locals_count++;
        free(keys);
        free(lengths);
    }
    return 0;
}

int CodeGen_Program(const void *this, FILE *out) {
    state.declared_vars = new_Map(0, 0);
    state.indent = 0;
    state.locals_count = 0;
    fprintf_code(out, "#include <stdlib.h>\n");
    fprintf_code(out, "#include <stdio.h>\n");
    fprintf_code(out, "\n");
    const ASTNode  *node = this;
    ASTProgramData *data = node->data;
    declare_locals(out, data->symbols);
    fprintf_code(out, "\n");
    fprintf_code(out, "int main() {\n");
    state.indent++;
    int size = 0;
    ASTNode **stmt_arr = NULL;
    safe_method_call(data->statements, array, &size, &stmt_arr);
    int err = 0;
    for (int i = 0; i < size; i++) {
        ASTStatementVTable *vtable = stmt_arr[i]->vtable;
        err = vtable->codegen(stmt_arr[i], out) || err;
    }
    free(stmt_arr);
    state.indent--;
    fprintf_code(out, "}\n");
    return 0;
}

int CodeGen_Assignment(const void *this, FILE *out) {
    const ASTNode     *node = this;
    ASTAssignmentData *data = node->data;
    const ASTNode     *lhs_node = data->lhs;
    ASTLExprVTable    *lhs_vtable = lhs_node->vtable;
    if (lhs_vtable->codegen(lhs_node, out)) {
        return 1;
    }
    return 0;
}

int CodeGen_Return(const void *this, FILE *out) {
    fprintf(stderr, "Return code gen under construction\n");
    return 0;
}

int CodeGen_Expression(const void *this, FILE *out) {
    fprintf(stderr, "Expression code gen under construction\n");
    return 0;
}

int CodeGen_Ref(const void *this, FILE *out) {
    fprintf(stderr, "Ref code gen under construction\n");
    return 0;
}

int CodeGen_Paren(const void *this, FILE *out) {
    fprintf(stderr, "Paren code gen under construction\n");
    return 0;
}

int CodeGen_Call(const void *this, FILE *out) {
    fprintf(stderr, "Call code gen under construction\n");
    return 0;
}

int CodeGen_Function(const void *this, FILE *out) {
    fprintf(stderr, "Function code gen under construction\n");
    return 0;
}

int CodeGen_Class(const void *this, FILE *out) {
    const ASTNode *node = this;
    ASTClassData  *data = node->data;

    return 0;
}

int CodeGen_Int(const void *this, FILE *out) {
    fprintf(stderr, "Int code gen under construction\n");
    return 0;
}

int CodeGen_Double(const void *this, FILE *out) {
    fprintf(stderr, "Double code gen under construction\n");
    return 0;
}

int CodeGen_Variable(const void *this, FILE *out) {
    const ASTNode   *node = this;
    ASTVariableData *data = node->data;
    if (data->type == NULL) {
        print_ICE(
            "type checker failed to infer type of variable before code gen");
        return 1;
    }

    return 0;
}

int CodeGen_TypedVar(const void *this, FILE *out) {
    fprintf(stderr, "TypedVar code gen under construction\n");
    return 0;
}
