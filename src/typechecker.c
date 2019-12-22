#include <stdlib.h>
#include <string.h>
#include "typechecker.h"
#include "ast.h"
#include "safe.h"
#include "ast/program.h"
#include "builtins.h"

static int funccmp(const FuncType *type1,
                   const FuncType *type2,
                   TypeCheckState *state,
                   const Map *symbols,
                   const Map *seen);
static int copy_FuncType(FuncType *type, FuncType **copy_ptr);
static int copy_TraitType(ClassType *type, ClassType **copy_ptr);
static int copy_ClassType(ClassType *type, ClassType **copy_ptr);
static int copy_ObjectType(ObjectType *type, ObjectType **copy_ptr);

int getObjectClass(ObjectType *object,
                   const Map *symbols,
                   const Vector *classTypes,
                   ClassType **type_ptr) {
    if (object->id_type == ID) {
        return classTypes->get(classTypes, object->classID, type_ptr);
    } else if (object->id_type == NAME) {
        VarType *type = NULL;
        if (symbols->get(symbols,
                         object->name,
                         strlen(object->name),
                         &type)) {
            return 1;
        }
        if (type->type != CLASS) {
            return 1;
        }
        *type_ptr = type->class;
        return 0;
    } else {
        print_ICE("ill-formed object type");
    }
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
        case TRAIT:
            free_ClassType(type->class);
            break;
        case TUPLE:
            type->tuple->free(type->tuple, free_VarType);
            break;
        case REFERENCE:
        case PAREN:
        case ERROR:
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

int new_ErrorType(VarType **vartype_ptr, VarType *sub_type) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = ERROR;
    (*vartype_ptr)->sub_type = sub_type;
    (*vartype_ptr)->is_ref = 0;
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
    (*vartype_ptr)->class->is_trait = 0;
    if (new_ObjectType(&(*vartype_ptr)->class->instance)) {
        free((*vartype_ptr)->class);
        free(*vartype_ptr);
    }
    (*vartype_ptr)->class->supers = new_Vector(0);
    return 0;
}

int new_TraitType(VarType **vartype_ptr) {
    if (vartype_ptr == NULL) {
        return 1;
    }
    *vartype_ptr = malloc(sizeof(**vartype_ptr));
    if (*vartype_ptr == NULL) {
        return 1;
    }
    (*vartype_ptr)->type = TRAIT;
    (*vartype_ptr)->is_ref = 0;
    (*vartype_ptr)->class = malloc(sizeof(ClassType));
    if ((*vartype_ptr)->class == NULL) {
        free(*vartype_ptr);
        return 1;
    }
    (*vartype_ptr)->class->field_name_to_type = new_Map(0, 0);
    (*vartype_ptr)->class->classID = -1;
    (*vartype_ptr)->class->is_trait = 1;
    (*vartype_ptr)->class->instance = NULL;
    (*vartype_ptr)->class->supers = NULL;
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

int new_FuncType(const Vector *args, VarType *ret_type, VarType **vartype_ptr) {
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
            case ERROR:
                printf("errorable ");
                print_VarType(type->sub_type);
                break;
            case HOLD:
                printf("hold ");
                print_VarType(type->sub_type);
                break;
            case CLASS:
                printf("class%d", type->class->classID);
                break;
            case TRAIT:
                printf("trait%d", type->class->classID);
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
        case TRAIT: safe_function_call(copy_TraitType,
                                       VT_type->class,
                                       &(*VT_copy_ptr)->class);
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
        safe_function_call(safe_strdup, &(*copy)->name, arg->name);
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

static int copy_TraitType(ClassType *type, ClassType **copy_ptr) {
    if (copy_ptr == NULL) {
        return 1;
    }
    *copy_ptr = malloc(sizeof(**copy_ptr));
    if (*copy_ptr == NULL) {
        return 1;
    }
    (*copy_ptr)->classID = type->classID;
    safe_method_call(type->field_name_to_type,
                     copy,
                     &(*copy_ptr)->field_name_to_type,
                     copy_VarType);
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
    safe_method_call(type->supers,
                     copy,
                     &(*copy_ptr)->supers,
                     NULL); // Vector of ints, so assignment works without copy
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

int classcmp(ClassType *type1,
             ClassType *type2,
             TypeCheckState *state,
             const Map *symbols,
             const Map *seen) {
    if (type1 == type2) {
        return 0;
    }
    size_t ID1 = type1->classID, ID2 = type2->classID;
    if (seen == NULL) {
        seen = new_Map(0, 0);
    } else {
        if (seen->contains(seen, &ID1, sizeof(ID1)) ||
            seen->contains(seen, &ID2, sizeof(ID2))) {
            return 1;
        }
    }
    safe_method_call(seen, put, &ID1, sizeof(ID1), NULL, NULL);
    safe_method_call(seen, put, &ID2, sizeof(ID2), NULL, NULL);
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
        if (typecmp(given_type, expected_type, state, symbols, seen)) {
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
                           const Map *symbols,
                           ClassType **class) {
    ASTProgramData *data = state->program_node->data;
    if (obj->id_type == ID) {
        return data->class_types->get(data->class_types, obj->classID, class);
    } else if (obj->id_type == NAME) {
        VarType *type = NULL;
        if (symbols->get(data->symbols,
                         obj->name,
                         strlen(obj->name),
                         &type)) {
            return 1;
        }
        if (type->type != CLASS && type->type != TRAIT) {
            print_ICE("ill-formatted object");
        }
        return data->class_types->get(data->class_types,
                                      type->class->classID,
                                      class);
    } else {
        print_ICE("ill-formed object type");
    }
}

int objectcmp(ObjectType *type1,
              ObjectType *type2,
              TypeCheckState *state,
              const Map *symbols,
              const Map *seen) {
    ClassType *class1 = NULL, *class2 = NULL;
    if (getClassTypeFromObject(type1, state, symbols, &class1) ||
        getClassTypeFromObject(type2, state, symbols, &class2)) {
        return 1;
    }
    return classcmp(class1, class2, state, symbols, seen);
}

int typecmp(const VarType *type1,
            const VarType *type2,
            TypeCheckState *state,
            const Map *symbols,
            const Map *seen) {
    //Returns 1 if the types differ, 0 if they match
    int delSeen, ret;
    if (type1 == NULL || type2 == NULL) {
        return 1;
    }
    if (type2->type == ERROR) {
        type2 = type2->sub_type;
    }
    if (type2->type == REFERENCE) {
        if (type1->is_ref) {
            type2 = type2->sub_type;
        } else {
            return 1;
        }
    }
    if (type1->type == REFERENCE) {
        type1 = type1->sub_type;
    }
    if (type1->type != type2->type) {
        return 1;
    }
    switch (type1->type) {
        case FUNCTION:
            return funccmp(type1->function,
                           type2->function,
                           state,
                           symbols,
                           seen);
        case OBJECT:
            delSeen = 0;
            if (seen == NULL) {
                delSeen = 1;
                seen = new_Map(0, 0);
            }
            ret = objectcmp(type1->object, type2->object, state, symbols, seen);
            if (delSeen) seen->free(seen, NULL);
            return ret;
        case CLASS:
        case TRAIT:
            delSeen = 0;
            if (seen == NULL) {
                delSeen = 1;
                seen = new_Map(0, 0);
            }
            ret = classcmp(type1->class, type2->class, state, symbols, seen);
            if (delSeen) seen->free(seen, NULL);
            return ret;
        case REFERENCE:
        case PAREN:
            return typecmp(type1->sub_type,
                           type1->sub_type,
                           state,
                           symbols,
                           seen);
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
                if (typecmp(sub1, sub2, state, symbols, seen)) {
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

int is_bool(VarType *type) {
    //Returns 1 if type is bool, 0 otherwise
    if (type->type != OBJECT) {
        return 0;
    }
    switch(type->object->id_type) {
        case ID:
            return type->object->classID == BOOL;
        case NAME:
            return !strcmp(type->object->name, BUILTIN_NAMES[BOOL]);
    }
    print_ICE("switch statement fell through");
}

static int funccmp(const FuncType *type1,
                   const FuncType *type2,
                   TypeCheckState *state,
                   const Map *symbols,
                   const Map *seen) {
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
            if (typecmp(arg1->type, arg2->type, state, symbols, seen)) {
                ret = 1;
                break;
            }
        }
    }
    return ret ||
           typecmp(type1->ret_type, type2->ret_type, state, symbols, seen);
}

int typeIntersection(VarType *type1,
                     VarType *type2,
                     const Map *symbols,
                     TypeCheckState *state,
                     VarType **type_ptr) {
    if (!typecmp(type1,
                 type2,
                 state,
                 symbols,
                 NULL) &&
        !typecmp(type2,
                 type1,
                 state,
                 symbols,
                 NULL)) {
        // Are type1 and type2 the same type?
        safe_function_call(copy_VarType,
                           type1,
                           type_ptr);
        return 0;
    }
    if (type1->type == OBJECT &&
        type2->type == OBJECT) {
        ClassType *class1 = NULL, *class2 = NULL;
        ASTProgramData *program_data = state->program_node->data;
        safe_function_call(getObjectClass,
                           type1->object,
                           symbols,
                           program_data->class_types,
                           &class1);
        safe_function_call(getObjectClass,
                           type2->object,
                           symbols,
                           program_data->class_types,
                           &class2);
        if (class1->classID == class2->classID) {
            safe_function_call(copy_VarType,
                               type1,
                               type_ptr);
            return 0;
        } else if (!typecmp(type1,
                            type2,
                            state,
                            symbols,
                            NULL)) {
            // Is type1 a subtype of type2?
            // Then type2 is the intersection of the two.
            safe_function_call(copy_VarType,
                               type2,
                               type_ptr);
            return 0;
        } else if (!typecmp(type2,
                            type1,
                            state,
                            symbols,
                            NULL)) {
            // Is type2 a subtype of type1?
            // Then type1 is the intersection of the two.
            safe_function_call(copy_VarType,
                               type1,
                               type_ptr);
            return 0;
        } else {
            // Neither type is a subtype of the other, find
            // their intersection manually.
            ClassType *new_trait = malloc(sizeof(*new_trait));
            new_trait->is_trait = 1;
            new_trait->classID = program_data->class_types->size(
                    program_data->class_types);
            new_trait->field_name_to_type = new_Map(0, 0);
            safe_method_call(program_data->class_types,
                             append,
                             new_trait);
            size_t num_fields, *field_lengths = NULL;
            char **field_names = NULL;
            safe_method_call(class1->field_name_to_type,
                             keys,
                             &num_fields,
                             &field_lengths,
                             &field_names);
            for (size_t j = 0; j < num_fields; j++) {
                VarType *field_type2 = NULL;
                if (!class2->field_name_to_type->get(
                        class2->field_name_to_type,
                        field_names[j],
                        field_lengths[j],
                        &field_type2)) {
                    VarType *field_type1 = NULL;
                    safe_method_call(class1->field_name_to_type,
                                     get,
                                     field_names[j],
                                     field_lengths[j],
                                     &field_type1);
                    VarType *field_intersection = NULL;
                    if (!typeIntersection(field_type1, field_type2, symbols,
                            state, &field_intersection)) {
                        safe_method_call(new_trait->field_name_to_type,
                                         put,
                                         field_names[j],
                                         field_lengths[j],
                                         field_intersection,
                                         NULL);
                    }
                }

            }
            safe_function_call(new_ObjectType, type_ptr);
            (*type_ptr)->object->id_type = ID;
            (*type_ptr)->object->classID = new_trait->classID;
            return 0;
        }
    }
    return 1;
}
