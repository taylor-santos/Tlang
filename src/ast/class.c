#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "typechecker.h"
#include "safe.h"
#include "Tlang_parser.h"
#include "ast/class.h"
#include "ast/program.h"
#include "ast/statement.h"
#include "codegen.h"

static void free_class(const void *this) {
    if (this == NULL) {
        return;
    }
    const ASTNode *node = this;
    ASTClassData  *data = node->data;
    free_VarType(data->type);
    data->supers->free(data->supers, free);
    data->stmts->free(data->stmts, free_ASTNode);
    data->symbols->free(data->symbols, free_VarType);
    data->fields->free(data->fields, NULL);
    free(data->loc);
    free(node->data);
    free(node->vtable);
    free((void *) node);
}

static void json_class(const void *this, int indent, FILE *out) {
    fprintf(out, "{\n");
    indent++;
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"type\": \"Class\",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    const ASTNode *node = this;
    ASTClassData  *data = node->data;
    fprintf(out,
            "\"loc\": \"%d:%d-%d:%d\",\n",
            data->loc->first_line,
            data->loc->first_column,
            data->loc->last_line,
            data->loc->last_column);
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"extends\": ");
    json_vector(data->supers, indent, out, json_string);
    fprintf(out, ",\n");
    fprintf(out, "%*s", indent * JSON_TAB_WIDTH, "");
    fprintf(out, "\"statements\": ");
    json_vector(data->stmts, indent, out, json_ASTNode);
    fprintf(out, "\n");
    indent--;
    fprintf(out, "%*s}", indent * JSON_TAB_WIDTH, "");
}

static int GetType_Class(const ASTNode *node,
                         const Map *symbols,
                         TypeCheckState *state,
                         VarType **type_ptr) {
    if (type_ptr == NULL) {
        return 1;
    }
    ASTClassData *data = node->data;
    const ASTNode *program_ast = state->program_node;
    ASTProgramData *program_data = program_ast->data;
    int classID = program_data->class_types->size(program_data->class_types);

    // Copy current symbol table into class environment
    safe_method_call(symbols, copy, &data->symbols, copy_VarType);
    safe_method_call(data->symbols, copy, &data->env, copy_VarType);

    // Initialize class's type object
    safe_function_call(new_ClassType, &data->type);
    data->type->class->classID = classID;
    data->type->class->instance->object->id_type = ID;
    data->type->class->instance->object->classID = classID;
    *type_ptr = data->type;

    // Update program data with new class information
    safe_method_call(program_data->class_types, append, data->type->class);
    safe_method_call(program_data->class_stmts, append, data->stmts);
    safe_method_call(program_data->class_envs,  append, data->env);

    // Add "self" object to class's symbol table
    {
        VarType *self_type = NULL;
        safe_function_call(copy_VarType, data->type, &self_type);
        self_type->is_ref = 0;
        VarType *prev_self = NULL;
        char *self_name = "self";
        safe_method_call(data->symbols,
                         put,
                         self_name,
                         strlen(self_name),
                         self_type,
                         &prev_self);
        if (prev_self != NULL) {
            free_VarType(prev_self);
        }
    }
    {
        VarType *this_type = NULL;
        safe_function_call(copy_VarType,
                           data->type->class->instance,
                           &this_type);
        this_type->is_ref = 0;
        VarType *prev_this = NULL;
        char *this_name = "this";
        safe_method_call(data->symbols,
                         put,
                         this_name,
                         strlen(this_name),
                         this_type,
                         &prev_this);
        if (prev_this != NULL) {
            free_VarType(prev_this);
        }
    }
    if (state->curr_class != NULL) {
        ASTClassData *super_data = state->curr_class->data;
        VarType *super_type = NULL;
        safe_function_call(copy_VarType,
                           super_data->type->class->instance,
                           &super_type);
        super_type->is_ref = 0;
        VarType *prev_super = NULL;
        char *super_name = "super";
        safe_method_call(data->symbols,
                         put,
                         super_name,
                         strlen(super_name),
                         super_type,
                         &prev_super);
        if (prev_super != NULL) {
            free_VarType(prev_super);
        }
    }

    // Iterate through parent classes, collecting their fields
    size_t super_count = data->supers->size(data->supers);
    data->type->class->supers = new_Vector(0); // Vector of class IDs
    for (size_t i = 0; i < super_count; i++) {
        char *super_name = NULL;
        safe_method_call(data->supers, get, i, &super_name);
        VarType *super = NULL;
        if(symbols->get(symbols, super_name, strlen(super_name), &super)) {
            //TODO: Handle unrecognized super name
            fprintf(stderr, "error: class inherits from unrecognized type\n");
            return 1;
        }
        if (super->type != CLASS) {
            //TODO: Handle inherits from non-class type
            fprintf(stderr, "error: class inherits from non-class type\n");
            return 1;
        }
        size_t superID = super->class->classID;
        safe_method_call(data->type->class->supers, append, (void*)superID);
        ClassType *super_class = super->class;
        size_t field_count = 0;
        size_t *field_lengths = NULL;
        char **field_names = NULL;
        safe_method_call(super_class->field_name_to_type,
                         keys,
                         &field_count,
                         &field_lengths,
                         &field_names);
        for (size_t j = 0; j < field_count; j++) {
            VarType *field_type = NULL;
            safe_method_call(super_class->field_name_to_type,
                             get,
                             field_names[j],
                             field_lengths[j],
                             &field_type);
            VarType *prev_type = NULL;
            VarType *type_copy = NULL;
            safe_function_call(copy_VarType, field_type, &type_copy);

            // Inherited fields can completely override variables from the
            // environment
            safe_method_call(data->symbols,
                             put,
                             field_names[j],
                             field_lengths[j],
                             field_type,
                             &prev_type);

            safe_function_call(copy_VarType, field_type, &type_copy);
            prev_type = NULL;
            safe_method_call(data->type->class->field_name_to_type,
                             put,
                             field_names[j],
                             field_lengths[j],
                             type_copy,
                             &prev_type);
            if (prev_type != NULL &&
                typecmp(type_copy, prev_type, state, symbols, NULL)) {
                //TODO: Handle incompatible fields
                fprintf(stderr, "error: field \"%.*s\" inherited from class "
                                "\"%s\" shadows existing field\n",
                        (int)field_lengths[j],
                        field_names[j],
                        super_name);
                return 1;
            }
            free(prev_type);
            free(field_names[j]);
        }
        free(field_names);
        free(field_lengths);
    }
    const Vector *stmts = data->stmts;
    const ASTNode *prev_class = state->curr_class;
    state->curr_class = node;
    size_t stmt_count = stmts->size(stmts);
    for (size_t i = 0; i < stmt_count; i++) {
        const ASTNode *stmt = NULL;
        safe_method_call(stmts, get, i, &stmt);
        ASTStatementVTable *vtable = stmt->vtable;
        VarType *type = NULL;
        if (vtable->get_type(stmt,
                             data->symbols,
                             state,
                             &type)) {
            return 1;
        }
    }
    state->curr_class = prev_class;
    //Clear the new_symbols vector without freeing its elements. They have been
    //moved to the class's fields list.
    return 0;
}

static char *CodeGen_Class(const ASTNode *node,
                           CodegenState *state,
                           FILE *out) {
    ASTClassData *data = node->data;
    VarType *type = data->type;
    ClassType *class = type->class;
    char *ret;
    safe_asprintf(&ret, "tmp%d", state->tmp_count++);
    print_indent(state->indent, out);
    fprintf(out, "build_closure(%s, new_class%d", ret, class->classID);
    const char **symbols = NULL;
    size_t *lengths, symbol_count;
    safe_method_call(data->env, keys, &symbol_count, &lengths, &symbols);
    for (size_t j = 0; j < symbol_count; j++) {
        VarType *var_type = NULL;
        safe_method_call(data->symbols, get, symbols[j], lengths[j], &var_type);
        if (var_type->type != TRAIT) {
            fprintf(out, ", var_%.*s", (int) lengths[j], symbols[j]);
        }
        free((void *) symbols[j]);
    }
    free(lengths);
    free(symbols);
    fprintf(out, ")\n");
    return ret;
}

const ASTNode *new_ClassNode(struct YYLTYPE *loc,
                             const Vector *supers,
                             const Vector *stmts) {
    ASTNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    struct ast_class_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        free(node);
        return NULL;
    }
    node->data = data;
    struct ast_class_vtable *vtable = malloc(sizeof(*vtable));
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
    data->supers      = supers;
    data->stmts       = stmts;
    data->fields      = new_Map(0, 0);
    data->env         = new_Map(0, 0);
    data->type        = NULL;
    data->symbols     = NULL;
    data->name        = NULL;
    data->is_hold     = 0;
    vtable->free      = free_class;
    vtable->json      = json_class;
    vtable->get_type  = GetType_Class;
    vtable->codegen   = CodeGen_Class;
    return node;
}
