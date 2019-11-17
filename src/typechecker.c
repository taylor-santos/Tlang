#include <stdlib.h>
#include <string.h>
#include "typechecker.h"
#include "ast.h"
#include "safe.h"
#include "builtins.h"
#include "ast/program.h"

static int funccmp(const FuncType *type1,
                   const FuncType *type2,
                   TypeCheckState *state);
static int copy_FuncType(FuncType *type, FuncType **copy_ptr);
static int copy_ClassType(ClassType *type, ClassType **copy_ptr);
static int copy_ObjectType(ObjectType *type, ObjectType **copy_ptr);

int add_builtins(const ASTNode *node) {
    ASTProgramData *data = node->data;
    for (size_t classID = 0; classID < BUILTIN_COUNT; classID++) {
        char *class_name = NULL;
        safe_asprintf(&class_name, "var_%s", BUILTIN_NAMES[classID]);
        safe_method_call(data->class_index,
                         put,
                         class_name,
                         strlen(class_name),
                         (void*)classID,
                         NULL);
        ClassType *class_type = malloc(sizeof(*class_type));
        if (class_type == NULL) {
            print_ICE("unable to allocate memory");
        }
        class_type->classID = classID;
        class_type->field_name_to_type = new_Map(0, 0);
        {
            VarType *object_type = malloc(sizeof(VarType));
            if (object_type == NULL) {
                print_ICE("unable to allocate memory");
            }
            object_type->type = OBJECT;
            object_type->object = malloc(sizeof(ObjectType));
            if (object_type->object == NULL) {
                print_ICE("unable to allocate memory");
            }
            object_type->object->id_type = ID;
            object_type->object->classID = classID;
            safe_method_call(class_type->field_name_to_type,
                             put,
                             "var_val",
                             strlen("var_val"),
                             object_type,
                             NULL);
        }
        {
            const Vector *args = new_Vector(0);
            VarType *ret_type = NULL;
            safe_function_call(new_ObjectType, &ret_type);
            ret_type->object->id_type = NAME;
            ret_type->object->name = strdup("var_string");
            VarType *toString_type = NULL;
            safe_function_call(new_FuncType, args, ret_type, &toString_type);
            safe_method_call(class_type->field_name_to_type,
                             put,
                             "var_toString",
                             strlen("var_toString"),
                             toString_type,
                             NULL);
        }

        safe_method_call(data->class_types, append, class_type);
        VarType *type_copy = malloc(sizeof(*type_copy));
        type_copy->type = CLASS;
        type_copy->class = class_type;
        safe_method_call(data->symbols,
                         put,
                         class_name,
                         strlen(class_name),
                         type_copy,
                         NULL);
    }
    {
		char *trait_name = strdup("var_stringable");
		size_t classID = data->class_types->size(data->class_types);
        safe_method_call(data->class_index,
                         put,
                         trait_name,
                         strlen(trait_name),
                         (void*)classID,
                         NULL);
        ClassType *class_type = malloc(sizeof(*class_type));
        if (class_type == NULL) {
            print_ICE("unable to allocate memory");
        }
        class_type->classID = classID;
        class_type->field_name_to_type = new_Map(0, 0);
        const Vector *args = new_Vector(0);
        VarType *ret_type = NULL;
        safe_function_call(new_ObjectType, &ret_type);
        ret_type->object->id_type = NAME;
        ret_type->object->name = strdup("var_string");
        VarType *func_type = NULL;
        safe_function_call(new_FuncType, args, ret_type, &func_type);
        char *name = "var_toString";
        safe_method_call(class_type->field_name_to_type,
                         put,
                         name,
                         strlen(name),
                         func_type,
                         NULL);
        safe_method_call(data->class_types, append, class_type);
    }
    {
        VarType *arg_type = NULL;
        safe_function_call(new_ObjectType, &arg_type);
        arg_type->object->id_type = NAME;
        arg_type->object->name = strdup("var_stringable");
        NamedType *arg = NULL;
        safe_function_call(new_NamedType, strdup("var_val"), arg_type, &arg);
        const Vector *args = new_Vector(0);
        safe_method_call(args, append, arg);
        VarType *func_type = NULL;
        safe_function_call(new_FuncType, args, NULL, &func_type);
        safe_method_call(data->symbols,
                         put,
                         "var_println",
                         strlen("var_println"),
                         func_type,
                         NULL);
    }
    return 0;
}

void free_VarType(void *this) {
    if (this == NULL) {
        return;
    }
    VarType *type = this;
    switch (type->type) {
        case FUNCTION:
            free_FuncType(type->function);
            break;
        case OBJECT:
            free_ObjectType(type->object);
            break;
        case CLASS:
            free_ClassType(type->class);
            break;
        case TUPLE:
            type->tuple->free(type->tuple, free_VarType);
            break;
        case REFERENCE:
        case PAREN:
            free_VarType(type->sub_type);
            break;
        case HOLD:
            break;
    }
    free(this);
}

void free_FuncType(void *this) {
    FuncType *type = this;
    type->named_args->free(type->named_args, free_NamedType);
    free_VarType(type->ret_type);
    free(this);
}

void free_ClassType(void *this) {
    ClassType *type = this;
    type->field_name_to_type->free(type->field_name_to_type, free_VarType);
    free(this);
}

void free_ObjectType(void *this) {
    free(this);
}

void free_NamedType(void *this) {
    NamedType *arg = this;
    free(arg->name);
    free_VarType(arg->type);
    free(this);
}

int new_RefType(VarType **vartype_ptr, VarType *sub_type) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = REFERENCE;
    (*vartype_ptr)->sub_type = sub_type;
    (*vartype_ptr)->is_ref = 0;
    sub_type->is_ref = 1;
    return 0;
}

int new_TupleType(VarType **vartype_ptr, const Vector *types) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = TUPLE;
    (*vartype_ptr)->tuple = types;
    (*vartype_ptr)->is_ref = 0;
    return 0;
}

int new_HoldType(VarType **vartype_ptr) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = HOLD;
    (*vartype_ptr)->is_ref = 0;
    return 0;
}

int new_ClassType(VarType **vartype_ptr) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = CLASS;
    (*vartype_ptr)->is_ref = 0;
    (*vartype_ptr)->class = malloc(sizeof(ClassType));
    if ((*vartype_ptr)->class == NULL) {
        free(*vartype_ptr);
        return 1;
    }
    (*vartype_ptr)->class->field_name_to_type = new_Map(0, 0);
    (*vartype_ptr)->class->classID = -1;
    if (new_ObjectType(&(*vartype_ptr)->class->instance)) {
        free((*vartype_ptr)->class);
        free(*vartype_ptr);
    }
    return 0;
}

int new_ObjectType(VarType **vartype_ptr) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = OBJECT;
    (*vartype_ptr)->object = malloc(sizeof(ObjectType));
    if ((*vartype_ptr)->object == NULL) {
        free(*vartype_ptr);
        return 1;
    }
    (*vartype_ptr)->object->id_type = -1;
    (*vartype_ptr)->is_ref = 0;
    return 0;
}

int new_ParenType(VarType **vartype_ptr) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = PAREN;
    (*vartype_ptr)->is_ref = 0;
    return 0;
}

int new_NamedType(char *name, VarType *type, NamedType **namedarg_ptr) {
    if (namedarg_ptr == NULL) {
        return 1;
    }
    *namedarg_ptr = malloc(sizeof(**namedarg_ptr));
    if (*namedarg_ptr == NULL) {
        return 1;
    }
    (*namedarg_ptr)->name = name;
    (*namedarg_ptr)->type = type;
    return 0;
}

int new_FuncType(
        const Vector *args, VarType *ret_type, VarType **vartype_ptr
) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = FUNCTION;
    (*vartype_ptr)->is_ref = 0;
    (*vartype_ptr)->function = malloc(sizeof(FuncType));
    if ((*vartype_ptr)->function == NULL) {
        free(*vartype_ptr);
        return 1;
    }
    (*vartype_ptr)->function->named_args = args;
    (*vartype_ptr)->function->ret_type = ret_type;
    return 0;
}

void print_VarType(const void *this) {
    const VarType *type = this;
    if (type == NULL) {
        printf("none");
    } else {
        if (type->is_ref) printf("ref ");
        switch (type->type) {
            case FUNCTION:
                printf("fn(");
                FuncType *func = type->function;
                int n = func->named_args->size(func->named_args);
                char *sep = "";
                for (int i = 0; i < n; i++) {
                    NamedType *arg = NULL;
                    safe_method_call(func->named_args, get, i, &arg);
                    printf("%s", sep);
                    if (arg->name != NULL) {
                        printf("%s:", arg->name);
                    }
                    print_VarType(arg->type);
                    sep = ", ";
                }
                printf(")->");
                print_VarType(func->ret_type);
                break;
            case REFERENCE:
                printf("ref ");
                print_VarType(type->sub_type);
                break;
            case HOLD:
                printf("hold ");
                print_VarType(type->sub_type);
                break;
            case CLASS:
                printf("class%d", type->class->classID);
                break;
            case OBJECT:
                if (type->object->id_type == ID)
                    printf("object%d", type->object->classID);
                else
                    printf("<%s>", type->object->name);
                break;
            case TUPLE:
                printf("(");
                sep = "";
                size_t type_count = type->tuple->size(type->tuple);
                for (size_t i = 0; i < type_count; i++) {
                    VarType *element = NULL;
                    safe_method_call(type->tuple, get, i, &element);
                    printf("%s", sep);
                    print_VarType(element);
                    sep = ", ";
                }
                printf(")");
                break;
            case PAREN:
                printf("(");
                print_VarType(type->sub_type);
                printf(")");
                break;
        }
    }
}

int copy_VarType(const void *type, const void *copy_ptr) {
    VarType *VT_type = (VarType *) type;
    VarType **VT_copy_ptr = (VarType **) copy_ptr;
    if (VT_copy_ptr == NULL) {
        return 1;
    }
    if (VT_type == NULL) {
        *VT_copy_ptr = NULL;
        return 0;
    }
    *VT_copy_ptr = malloc(sizeof(**VT_copy_ptr));
    if (*VT_copy_ptr == NULL) {
        return 1;
    }
    (*VT_copy_ptr)->type = VT_type->type;
    (*VT_copy_ptr)->is_ref = VT_type->is_ref;
    switch (VT_type->type) {
        case FUNCTION: safe_function_call(copy_FuncType,
                                          VT_type->function,
                                          &(*VT_copy_ptr)->function);
            break;
        case OBJECT: safe_function_call(copy_ObjectType,
                                        VT_type->object,
                                        &(*VT_copy_ptr)->object);
            break;
        case CLASS: safe_function_call(copy_ClassType,
                                       VT_type->class,
                                       &(*VT_copy_ptr)->class);
            break;
        case REFERENCE:
        case HOLD:
        case PAREN: safe_function_call(copy_VarType,
                                       VT_type->sub_type,
                                       &(*VT_copy_ptr)->sub_type);
            break;
        case TUPLE: safe_method_call(VT_type->tuple,
                                     copy,
                                     &(*VT_copy_ptr)->tuple, copy_VarType);
            break;

    }
    return 0;
}

static int copy_NamedType(const void *this, const void *copy_ptr) {
    NamedType *arg = (NamedType *) this;
    NamedType **copy = (NamedType **) copy_ptr;
    if (copy == NULL) {
        return 1;
    }
    *copy = malloc(sizeof(**copy));
    safe_function_call(copy_VarType, arg->type, &(*copy)->type);
    if (arg->name != NULL) {
        safe_function_call(strdup_safe, arg->name, &(*copy)->name);
    } else {
        (*copy)->name = NULL;
    }
    return 0;
}

static int copy_FuncType(FuncType *type, FuncType **copy_ptr) {
    if (copy_ptr == NULL) {
        return 1;
    }
    *copy_ptr = malloc(sizeof(**copy_ptr));
    if (*copy_ptr == NULL) {
        return 1;
    }
    safe_method_call(type->named_args,
                     copy,
                     &(*copy_ptr)->named_args,
                     copy_NamedType);
    safe_function_call(copy_VarType, type->ret_type, &(*copy_ptr)->ret_type);
    return 0;
}

static int copy_ClassType(ClassType *type, ClassType **copy_ptr) {
    if (copy_ptr == NULL) {
        return 1;
    }
    *copy_ptr = malloc(sizeof(**copy_ptr));
    if (*copy_ptr == NULL) {
        return 1;
    }
    (*copy_ptr)->classID = type->classID;
    safe_function_call(copy_VarType, type->instance, &(*copy_ptr)->instance);
    safe_method_call(type->field_name_to_type,
                     copy,
                     &(*copy_ptr)->field_name_to_type,
                     copy_VarType);
    return 0;
}

static int copy_ObjectType(ObjectType *type, ObjectType **copy_ptr) {
    if (copy_ptr == NULL) {
        return 1;
    }
    *copy_ptr = malloc(sizeof(**copy_ptr));
    if (*copy_ptr == NULL) {
        return 1;
    }
    (*copy_ptr)->id_type = type->id_type;
    switch(type->id_type) {
        case ID:
            (*copy_ptr)->classID = type->classID;
            return 0;
        case NAME:
            (*copy_ptr)->name = strdup(type->name);
            if ((*copy_ptr)->name == NULL) {
                free(*copy_ptr);
                return 1;
            }
            return 0;
        default: print_ICE("ill-formed object type");
    }
}

int classcmp(ClassType *type1, ClassType *type2, UNUSED TypeCheckState *state) {
    if (type1 == type2) {
        return 0;
    }
    size_t field_count;
    size_t *field_lengths = NULL;
    char **fields = NULL;
    safe_method_call(type2->field_name_to_type,
            keys,
            &field_count,
            &field_lengths,
            &fields);
    int valid = 1;
    size_t i;
    for (i = 0; i < field_count; i++) {
        VarType *given_type = NULL;
        print_Map(type1->field_name_to_type, print_VarType);
        if (type1->field_name_to_type->get(type1->field_name_to_type,
                fields[i],
                field_lengths[i],
                &given_type)) {
            valid = 0;
            break;
        }
        VarType *expected_type = NULL;
        safe_method_call(type2->field_name_to_type,
                         get,
                         fields[i],
                         field_lengths[i],
                         &expected_type);
        if (typecmp(given_type, expected_type, state)) {
            valid = 0;
            break;
        }
        free(fields[i]);
    }
    for (; i < field_count; i++) {
        free(fields[i]);
    }
    free(field_lengths);
    free(fields);
    return !valid;
}

int getClassTypeFromObject(ObjectType *obj,
                           TypeCheckState *state,
                           ClassType **class) {
    ASTProgramData *data = state->program_node->data;
    if (obj->id_type == ID) {
        return data->class_types->get(data->class_types, obj->classID, class);
    } else if (obj->id_type == NAME) {
        size_t classID = -1;
        if (data->class_index->get(data->class_index,
                                   obj->name,
                                   strlen(obj->name),
                                   &classID)) {
            return 1;
        }
        return data->class_types->get(data->class_types, classID, class);
    } else {
        print_ICE("ill-formed object type");
    }
}

int objectcmp(ObjectType *type1,
              ObjectType *type2,
              UNUSED  TypeCheckState *state) {
    ClassType *class1 = NULL, *class2 = NULL;
    if (getClassTypeFromObject(type1, state, &class1) ||
        getClassTypeFromObject(type2, state, &class2)) {
        return 1;
    }
    return classcmp(class1, class2, state);
}

int typecmp(const VarType *type1, const VarType *type2, TypeCheckState *state) {
    //Returns 1 if the types differ, 0 if they match
    if (type1 == NULL || type2 == NULL) {
        return type1 != type2;
    }
    if (type2->type == REFERENCE) {
        if (type1->is_ref) {
            type2 = type2->sub_type;
        } else {
            return 1;
        }
    }
    if (type1->type != type2->type) {
        return 1;
    }
    switch (type1->type) {
        case FUNCTION:
            return funccmp(type1->function, type2->function, state);
        case OBJECT:
            return objectcmp(type1->object, type2->object, state);
        case CLASS:
            return classcmp(type1->class, type2->class, state);
        case REFERENCE:
        case PAREN:
            return typecmp(type1->sub_type, type1->sub_type, state);
        case HOLD:
            return 0;
        case TUPLE:;
            size_t len1 = type1->tuple->size(type1->tuple),
                   len2 = type2->tuple->size(type2->tuple);
            if (len1 != len2) {
                return 1;
            }
            for (size_t i = 0; i < len1; i++) {
                VarType *sub1 = NULL, *sub2 = NULL;
                safe_method_call(type1->tuple, get, i, &sub1);
                safe_method_call(type2->tuple, get, i, &sub2);
                if (typecmp(sub1, sub2, state)) {
                    return 1;
                }
            }
            return 0;
    }
    fprintf(stderr,
            "%s:%d: " RED "internal compiler error: " WHITE "switch "
            "statement failed to cover all cases\n",
            __FILE__,
            __LINE__);
    return 1;
}

static int funccmp(const FuncType *type1,
                   const FuncType *type2,
                   TypeCheckState *state) {
    int ret = 0;
    size_t size1 = type1->named_args->size(type1->named_args),
           size2 = type2->named_args->size(type2->named_args);
    if (size1 != size2) {
        ret = 1;
    } else {
        for (size_t i = 0; i < size1; i++) {
            NamedType *arg1 = NULL,
                      *arg2 = NULL;
            safe_method_call(type1->named_args, get, i, &arg1);
            safe_method_call(type2->named_args, get, i, &arg2);
            if (typecmp(arg1->type, arg2->type, state)) {
                ret = 1;
                break;
            }
        }
    }
    return ret || typecmp(type1->ret_type, type2->ret_type, state);
}
