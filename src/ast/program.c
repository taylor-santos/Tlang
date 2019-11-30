#include <stdlib.h>
#include <string.h>
#include <codegen.h>
#include "queue.h"
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
        {
            const Vector *args = new_Vector(0);
            VarType *arg_type = NULL;
            safe_function_call(new_ObjectType, &arg_type);
            arg_type->object->id_type = ID;
            arg_type->object->classID = classID;
            NamedType *arg = NULL;
            safe_function_call(new_NamedType, strdup("other"), arg_type, &arg);
            safe_method_call(args, append, arg);
            VarType *ret_type = NULL;
            safe_function_call(new_ObjectType, &ret_type);
            ret_type->object->id_type = ID;
            ret_type->object->classID = classID;
            VarType *func_type = NULL;
            safe_function_call(new_FuncType, args, ret_type, &func_type);
            safe_method_call(class_type->field_name_to_type,
                             put,
                             "0x3D",
                             strlen("0x3D"),
                             func_type,
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

typedef struct {
    void *value;
    int count;
} counter;

static int counterSort(const void *lhs, const void *rhs) {
    const counter *l = lhs, *r = rhs;
    return l->count - r->count;
}

static int counterSortRev(const void *lhs, const void *rhs) {
    const counter *l = lhs, *r = rhs;
    return r->count - l->count;
}

static int TypeCheck_Program(const ASTNode *node) {
    safe_function_call(add_builtins, node);
    ASTProgramData *data = node->data;
    const Vector *stmts = data->statements;
    TypeCheckState state;
    state.new_symbols = new_Vector(0);
    state.program_node = node;
    state.curr_ret_type = NULL;
    size_t num_stmts = stmts->size(stmts);
    for (size_t i = 0; i < num_stmts; i++) {
        const ASTNode *stmt = NULL;
        safe_method_call(stmts, get, i, &stmt);
        ASTStatementVTable *vtable = stmt->vtable;
        VarType *type = NULL;
        if (vtable->get_type(stmt, data->symbols, &state, &type)) {
            return 1;
        }
    }
    state.new_symbols->free(state.new_symbols, free_NamedType);
    size_t num_classes = data->class_types->size(data->class_types);
    unsigned char *is_a = calloc(num_classes * num_classes, sizeof(*is_a));
    counter *child_count = calloc(num_classes, sizeof(*child_count));
    if (is_a == NULL) {
        return 1;
    }
    for (size_t a = 0; a < num_classes; a++) {
        ClassType *A = NULL;
        safe_method_call(data->class_types, get, a, &A);
        for (size_t b = 0; b < num_classes; b++) {
            if (a == b) {
                is_a[num_classes * a + b] = 1;
            } else {
                ClassType *B = NULL;
                safe_method_call(data->class_types, get, b, &B);
                is_a[num_classes * a + b] = !classcmp(A,
                                                      B,
                                                      &state,
                                                      data->symbols,
                                                      NULL);
            }
        }
    }
    const Map **field_from = malloc(sizeof(*field_from) * num_classes);
    const Map **field_to = malloc(sizeof(*field_to) * num_classes);
    const Map **place = malloc(sizeof(*place) * num_classes);
    counter **from_size = malloc(sizeof(*from_size) * num_classes);
    if (field_to == NULL ||
        field_from == NULL ||
        place == NULL ||
        from_size == NULL) {
        return 1;
    }
    for (size_t i = 0; i < num_classes; i++) {
        child_count[i].value = (void*)i;
        for (size_t j = 0; j < num_classes; j++) {
            child_count[i].count += is_a[j * num_classes + i];
        }
        child_count[i].count--;
        place[i] = new_Map(0, 0);
        ClassType *class = NULL;
        safe_method_call(data->class_types, get, i, &class);
        char **field_names = NULL;
        size_t *field_lens = NULL;
        size_t num_fields = 0;
        safe_method_call(class->field_name_to_type,
                         keys,
                         &num_fields,
                         &field_lens,
                         &field_names);
        from_size[i] = calloc(num_fields, sizeof(**from_size));
        field_from[i] = new_Map(0, 0);
        field_to[i] = new_Map(0, 0);
        for (size_t j = 0; j < num_fields; j++) {
            safe_method_call(field_from[i],
                             put,
                             field_names[j],
                             field_lens[j],
                             new_Vector(0),
                             NULL);
            safe_method_call(field_to[i],
                             put,
                             field_names[j],
                             field_lens[j],
                             new_Vector(0),
                             NULL);
            safe_method_call(place[i],
                             put,
                             field_names[j],
                             field_lens[j],
                             (void*)-1,
                             NULL);
        }
    }
    for (size_t i = 0; i < num_classes; i++) {
        ClassType *class = NULL;
        safe_method_call(data->class_types, get, i, &class);
        char **field_names = NULL;
        size_t *field_lens = NULL;
        size_t num_fields = 0;
        safe_method_call(class->field_name_to_type,
                         keys,
                         &num_fields,
                         &field_lens,
                         &field_names);
        for (size_t j = 0; j < num_fields; j++) {
            VarType *field_type = NULL;
            safe_method_call(class->field_name_to_type,
                             get,
                             field_names[j],
                             field_lens[j],
                             &field_type);
            const Vector *from = NULL;
            safe_method_call(field_from[i],
                             get,
                             field_names[j],
                             field_lens[j],
                             &from);
            for (size_t fromID = 0; fromID < num_classes; fromID++) {
                if (fromID != i && is_a[num_classes * i + fromID]) {
                    ClassType *fromClass = NULL;
                    safe_method_call(data->class_types,
                                     get,
                                     fromID,
                                     &fromClass);
                    if (fromClass->field_name_to_type->contains(
                            fromClass->field_name_to_type,
                            field_names[j],
                            field_lens[j])) {
                        safe_method_call(from, append, (void*)fromID);
                        const Vector *to = NULL;
                        safe_method_call(field_to[fromID],
                                         get,
                                         field_names[j],
                                         field_lens[j],
                                         &to);
                        safe_method_call(to, append, (void*)i);
                    }
                }
            }
            free(field_names[j]);
        }
        free(field_names);
        free(field_lens);
    }
    for (size_t i = 0; i < num_classes; i++) {
        ClassType *class = NULL;
        safe_method_call(data->class_types, get, i, &class);
        char **field_names = NULL;
        size_t *field_lens = NULL;
        size_t num_fields = 0;
        safe_method_call(class->field_name_to_type,
                         keys,
                         &num_fields,
                         &field_lens,
                         &field_names);
        for (size_t j = 0; j < num_fields; j++) {
            from_size[i][j].value = strndup(field_names[j], field_lens[j]);
            const Vector *from = NULL;
            safe_method_call(field_from[i],
                             get,
                             field_names[j],
                             field_lens[j],
                             &from);
            size_t from_count = from->size(from);
            for (size_t k = 0; k < from_count; k++) {
                size_t fromID = 0;
                safe_method_call(from, get, k, &fromID);
                from_size[i][j].count += child_count[fromID].count;
            }
        }
        qsort(from_size[i], num_fields, sizeof(*from_size[i]), counterSortRev);
    }
    qsort(child_count, num_classes, sizeof(*child_count), counterSort);
    for (size_t i = 0; i < num_classes; i++) {
        size_t c = (size_t)child_count[i].value;
        ClassType *class = NULL;
        safe_method_call(data->class_types, get, c, &class);
        size_t num_fields = class->field_name_to_type->size(
                class->field_name_to_type);
        for (size_t j = 0; j < num_fields; j++) {
            char *f = from_size[c][j].value;
            size_t f_len = strlen(f);
            size_t p = 0;
            safe_method_call(place[c], get, f, f_len, &p);
            if (p == (size_t)-1) {
                safe_method_call(place[c], put, f, f_len, (void*)-2, &p);
                const Vector *v = new_Vector(0);
                safe_method_call(v, append, (void*)c);
                size_t k = 0;
                while (v->size(v) > k) {
                    size_t l = 0;
                    safe_method_call(v, get, k, &l);
                    k++;
                    {
                        const Vector *to = NULL;
                        safe_method_call(field_to[l], get, f, f_len, &to);
                        size_t num_to = to->size(to);
                        for (size_t x = 0; x < num_to; x++) {
                            size_t t = 0;
                            safe_method_call(to, get, x, &t);
                            safe_method_call(place[t], get, f, f_len, &p);
                            if (p == (size_t) -1) {
                                safe_method_call(place[t],
                                                 put,
                                                 f,
                                                 f_len,
                                                 (void *) -2, &p);
                                safe_method_call(v, append, (void *) t);
                            }
                        }
                    }
                    {
                        const Vector *from = NULL;
                        safe_method_call(field_from[l], get, f, f_len, &from);
                        size_t num_from = from->size(from);
                        for (size_t x = 0; x < num_from; x++) {
                            size_t t = 0;
                            safe_method_call(from, get, x, &t);
                            safe_method_call(place[t], get, f, f_len, &p);
                            if (p == (size_t) -1) {
                                safe_method_call(place[t],
                                                 put,
                                                 f,
                                                 f_len,
                                                 (void *) -2, &p);
                                safe_method_call(v, append, (void *) t);
                            }
                        }
                    }
                }
                const Map *taken = new_Map(0, 0);
                size_t v_size = v->size(v);
                for (size_t x = 0; x < v_size; x++) {
                    size_t t = 0;
                    safe_method_call(v, get, x, &t);
                    char **keys = NULL;
                    size_t *key_lens = NULL, num_keys;
                    safe_method_call(place[t],
                                     keys,
                                     &num_keys,
                                     &key_lens,
                                     &keys);
                    for (size_t y = 0; y < num_keys; y++) {
                        safe_method_call(place[t],
                                         get,
                                         keys[y],
                                         key_lens[y],
                                         &p);
                        if (p != (size_t)-1 && p != (size_t)-2) {
                            safe_method_call(taken,
                                             put,
                                             &p,
                                             sizeof(p),
                                             (void*)p,
                                             &p);
                        }
                    }
                }
                size_t untaken = 0;
                while (taken->contains(taken, &untaken, sizeof(p))) {
                    untaken++;
                }
                for (size_t x = 0; x < v_size; x++) {
                    size_t t = 0;
                    safe_method_call(v, get, x, &t);
                    safe_method_call(place[t],
                                     put,
                                     f,
                                     f_len,
                                     (void*)untaken,
                                     &p);
                }
            }
        }
    }
    data->fields = malloc(num_classes * sizeof(*data->fields));
    data->field_counts = malloc(num_classes * sizeof(*data->field_counts));
    for (size_t i = 0; i < num_classes; i++) {
        size_t max = 0;
        char **keys = NULL;
        size_t *key_lens = NULL, num_keys;
        safe_method_call(place[i],
                         keys,
                         &num_keys,
                         &key_lens,
                         &keys);
        for (size_t j = 0; j < num_keys; j++) {
            size_t index = 0;
            safe_method_call(place[i], get, keys[j], key_lens[j], &index);
            if (index > max) max = index;
        }
        data->fields[i] = calloc(max + 1, sizeof(**data->fields));
        data->field_counts[i] = max + 1;
        for (size_t j = 0; j < num_keys; j++) {
            size_t index = 0;
            safe_method_call(place[i], get, keys[j], key_lens[j], &index);
            data->fields[i][index] = strndup(keys[j], key_lens[j]);
        }
    }
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
