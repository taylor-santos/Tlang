#include <stdlib.h>
#include <string.h>
#include <codegen.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/program.h"
#include "ast/statement.h"
#include "builtins.h"

static void free_program(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode  *node = this;
    ASTProgramData *data = node->data;
    data->statements->free(data->statements, free_ASTNode);
    data->symbols->free(data->symbols, free_VarType);
    data->func_defs->free(data->func_defs, free);
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

static int add_builtins(const ASTNode *node) {
    ASTProgramData *data = node->data;
    for (size_t classID = 0; classID < BUILTIN_COUNT; classID++) {
        char *class_name = NULL;
        safe_strdup(&class_name, BUILTIN_NAMES[classID]);
        ClassType *class_type = malloc(sizeof(*class_type));
        if (class_type == NULL) {
            print_ICE("unable to allocate memory");
        }
        class_type->classID = classID;
        class_type->field_name_to_type = new_Map(0, 0);
        class_type->is_trait = 0;
        class_type->supers = new_Vector(0);
        safe_function_call(new_ObjectType, &class_type->instance);
        class_type->instance->object->id_type = ID;
        class_type->instance->object->classID = classID;
        {
            VarType *object_type = NULL;
            if (new_ObjectType(&object_type)) return 1;
            object_type->object->id_type = ID;
            object_type->object->classID = classID;
            safe_method_call(class_type->field_name_to_type,
                             put,
                             "val",
                             strlen("val"),
                             object_type,
                             NULL);
        }
        {
            const Vector *args = new_Vector(0);
            VarType *ret_type = NULL;
            safe_function_call(new_ObjectType, &ret_type);
            ret_type->object->id_type = NAME;
            ret_type->object->name = strdup("string");
            VarType *toString_type = NULL;
            safe_function_call(new_FuncType, args, ret_type, &toString_type);
            safe_method_call(class_type->field_name_to_type,
                             put,
                             "toString",
                             strlen("toString"),
                             toString_type,
                             NULL);
        }

        safe_method_call(data->class_types, append, class_type);
        VarType *type_copy = malloc(sizeof(*type_copy));
        type_copy->type = CLASS;
        type_copy->class = class_type;
        type_copy->is_ref = 0;
        safe_method_call(data->symbols,
                         put,
                         class_name,
                         strlen(class_name),
                         type_copy,
                         NULL);
        safe_method_call(data->class_stmts,
                         append,
                         new_Vector(0));
        safe_method_call(data->class_envs,
                         append,
                         new_Map(0, 0));
    }
    {
        char *trait_name = strdup("stringable");
        size_t classID = data->class_types->size(data->class_types);
        ClassType *trait_type = malloc(sizeof(*trait_type));
        if (trait_type == NULL) {
            print_ICE("unable to allocate memory");
        }
        trait_type->classID = classID;
        trait_type->is_trait = 1;
        trait_type->field_name_to_type = new_Map(0, 0);
        trait_type->supers = new_Vector(0);
        const Vector *args = new_Vector(0);
        VarType *ret_type = NULL;
        safe_function_call(new_ObjectType, &ret_type);
        ret_type->object->id_type = NAME;
        ret_type->object->name = strdup("string");
        VarType *func_type = NULL;
        safe_function_call(new_FuncType, args, ret_type, &func_type);
        char *name = "toString";
        safe_method_call(trait_type->field_name_to_type,
                         put,
                         name,
                         strlen(name),
                         func_type,
                         NULL);
        safe_method_call(data->class_types, append, trait_type);
        VarType *type_copy = malloc(sizeof(*type_copy));
        type_copy->type = TRAIT;
        type_copy->class = trait_type;
        type_copy->is_ref = 0;
        safe_method_call(data->symbols,
                         put,
                         trait_name,
                         strlen(trait_name),
                         type_copy,
                         NULL);
        safe_method_call(data->class_stmts,
                         append,
                         NULL);
        safe_method_call(data->class_envs,
                         append,
                         NULL);
    }
    {
        VarType *arg_type = NULL;
        safe_function_call(new_ObjectType, &arg_type);
        arg_type->object->id_type = NAME;
        arg_type->object->name = strdup("stringable");
        NamedType *arg = NULL;
        safe_function_call(new_NamedType, strdup("val"), arg_type, &arg);
        const Vector *args = new_Vector(0);
        safe_method_call(args, append, arg);
        VarType *func_type = NULL;
        safe_function_call(new_FuncType, args, NULL, &func_type);
        safe_method_call(data->symbols,
                         put,
                         "println",
                         strlen("println"),
                         func_type,
                         NULL);
    }
    return 0;
}

static int TypeCheck_Program(const ASTNode *node) {
    safe_function_call(add_builtins, node);
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
    data->class_types  = new_Vector(0);
    data->class_stmts  = new_Vector(0);
    data->class_envs   = new_Vector(0);
    data->func_defs    = new_Map(0, 0);
    vtable->free       = free_program;
    vtable->json       = json_program;
    vtable->type_check = TypeCheck_Program;
    vtable->codegen    = CodeGen_Program;
    return node;
}
