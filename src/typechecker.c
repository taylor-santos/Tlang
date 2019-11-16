#include <stdlib.h>
#include <string.h>
#include "typechecker.h"
#include "ast.h"
#include "safe.h"
#include "builtins.h"

static int funccmp(const FuncType *type1, const FuncType *type2);
static int copy_FuncType(FuncType *type, FuncType **copy_ptr);
static int copy_ClassType(ClassType *type, ClassType **copy_ptr);
static int copy_ObjectType(ObjectType *type, ObjectType **copy_ptr);

int getObjectID(VarType *type, const Map *symbols) {
    switch (type->type) {
        case OBJECT:;
            ObjectType *object = type->object;
            if (object->className == NULL || object->classID != -1) {
                return 1;
            }
            VarType *classType = NULL;
            if (symbols->get(symbols,
                             object->className,
                             strlen(object->className),
                             &classType)) {
                return 1;
            }
            if (classType->type != CLASS) {
                return 1;
            }
            object->classID = classType->class->classID;
            break;
        case CLASS:;
            ClassType *class = type->class->def;
            if (class == type->class) {
                size_t      field_count = class->fields->size(class->fields);
                for (size_t i           = 0; i < field_count; i++) {
                    NamedType *field = NULL;
                    safe_method_call(class->fields, get, i, &field);
                    if (getObjectID(field->type, symbols)) {
                        return 1;
                    }
                }
            }
            break;
        case FUNCTION:;
            FuncType *func = type->function;
            size_t arg_count;
            NamedType **args = NULL;
            if (func->named_args->array(func->named_args, &arg_count, &args)) {
                return 1;
            }
            for (size_t i = 0; i < arg_count; i++) {
                if (getObjectID(args[i]->type, symbols)) {
                    return 1;
                }
            }
            free(args);
            if (func->ret_type != NULL &&
                getObjectID(func->ret_type, symbols)) {
                return 1;
            }
            break;
        case TUPLE:;
            size_t type_count;
            VarType **types = NULL;
            if (type->tuple->array(type->tuple, &type_count, &types)) {
                return 1;
            }
            for (size_t i = 0; i < type_count; i++) {
                if (getObjectID(types[i], symbols)) {
                    return 1;
                }
            }
            free(types);
            break;
        case REFERENCE:
        case PAREN:
            if (getObjectID(type->sub_type, symbols)) {
                return 1;
            }
            break;
        case HOLD:
            break;
    }
    return 0;
}

int add_builtins(const Map *symbols, const Vector *class_defs) {
    if (symbols == NULL) {
        return 1;
    }
    size_t n;
    const char *binops[] = { "+", "=" };
    n = sizeof(binops) / sizeof(*binops);

    for (size_t i = 0; i < BUILTIN_COUNT; i++) {
        VarType *class = NULL;
        safe_function_call(new_ClassType, &class);
        class->class->def = class->class;
        class->class->stmts = new_Vector(0);
        class->class->classID = i;
        class->class->instance->object->classID = i;
        ClassType *classType = class->class;
        classType->def = classType;
        classType->fields = new_Vector(0);
        classType->field_names = new_Map(0, 0);
        for (size_t j = 0; j < n; j++) {
            VarType *arg_type = NULL;
            safe_function_call(new_ObjectType, &arg_type);
            arg_type->object->classID = i;
            NamedType *arg = NULL;
            safe_function_call(new_NamedType, strdup("other"), arg_type, &arg);
            const Vector *args = new_Vector(0);
            safe_method_call(args, append, arg);
            VarType *ret_type = NULL;
            safe_function_call(new_ObjectType, &ret_type);
            ret_type->object->classID = i;
            VarType *func = NULL;
            safe_function_call(new_FuncType, args, ret_type, &func);
            char *name;
            asprintf(&name, "var_0x%X", binops[j][0]);
            size_t len = strlen(binops[j]);
            for (size_t k = 1; k < len; k++) {
                append_string(&name, "%X", binops[j][k]);
            }
            NamedType *field = NULL;
            safe_function_call(new_NamedType, name, func, &field);
            safe_method_call(classType->fields, append, field);
            safe_method_call(classType->field_names,
                             put,
                             field->name,
                             strlen(field->name),
                             func,
                             NULL);
        }
        safe_method_call(class_defs, append, class);
        char *name = NULL;
        safe_asprintf(&name, "var_%s", BUILTIN_NAMES[i]);
        safe_method_call(symbols, put, name, strlen(name), class, NULL);
        free(name);
    }

    const char *builtins[] = { "println" };
    n = sizeof(builtins) / sizeof(*builtins);
    for (size_t i = 0; i < n; i++) {
        VarType *arg_type = NULL;
        safe_function_call(new_ObjectType, &arg_type);
        arg_type->object->classID = INT;
        NamedType *arg = NULL;
        safe_function_call(new_NamedType, strdup("val"), arg_type, &arg);
        const Vector *args = new_Vector(0);
        safe_method_call(args, append, arg);
        VarType *func = NULL;
        safe_function_call(new_FuncType, args, NULL, &func);
        char *name = NULL;
        safe_asprintf(&name, "var_%s", builtins[i]);
        safe_method_call(symbols, put, name, strlen(name), func, NULL);
        free(name);
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
    if (type->def == type) {
        type->fields->free(type->fields, free_NamedType);
        type->field_names->free(type->field_names, NULL);
    }
    free_VarType(type->instance);
    free(this);
}

void free_ObjectType(void *this) {
    ObjectType *type = this;
    free(type->className);
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
    (*vartype_ptr)->class = malloc(sizeof(ClassType));
    if ((*vartype_ptr)->class == NULL) {
        free(*vartype_ptr);
        return 1;
    }
    (*vartype_ptr)->class->def = NULL;
    (*vartype_ptr)->class->classID = -1;
    if (new_ObjectType(&(*vartype_ptr)->class->instance)) {
        free((*vartype_ptr)->class);
        free(*vartype_ptr);
        return 1;
    }
    (*vartype_ptr)->class->instance->object->classID = -1;
    (*vartype_ptr)->is_ref = 0;
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
    (*vartype_ptr)->object->classID = -1;
    (*vartype_ptr)->object->className = NULL;
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
                printf("class%ld: {", type->class->classID);
                /*
                if (type->class->classID < BUILTIN_COUNT) {
                    printf("val:%s}", BUILTIN_NAMES[type->class->classID]);
                } else {*/
                ClassType *def = type->class->def;
                size_t field_count = def->fields->size(def->fields);
                sep = "";
                for (size_t i = 0; i < field_count; i++) {
                    NamedType *field = NULL;
                    safe_method_call(def->fields, get, i, &field);
                    printf("%s%s:", sep, field->name);
                    print_VarType(field->type);
                    sep = ", ";
                }
                printf("}");
                //}
                break;
            case OBJECT:
                printf("object%d", type->object->classID);
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
    (*copy_ptr)->def = type->def;
    if (copy_VarType(type->instance, &(*copy_ptr)->instance)) {
        free(copy_ptr);
        return 1;
    }
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
    (*copy_ptr)->classID = type->classID;
    (*copy_ptr)->className = type->className == NULL
                             ? NULL
                             : strdup(type->className);
    return 0;
}

int typecmp(const VarType *type1, const VarType *type2) {
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
            return funccmp(type1->function, type2->function);
        case OBJECT:
            return type1->object->classID != type2->object->classID;
        case CLASS:
            return type1->class->classID != type2->class->classID;
        case REFERENCE:
        case PAREN:
            return typecmp(type1->sub_type, type1->sub_type);
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
                if (typecmp(sub1, sub2)) {
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

static int funccmp(const FuncType *type1, const FuncType *type2) {
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
            if (typecmp(arg1->type, arg2->type)) {
                ret = 1;
                break;
            }
        }
    }
    return ret || typecmp(type1->ret_type, type2->ret_type);
}
