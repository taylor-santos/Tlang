#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    void *fn;
    void **env;
    size_t env_size;
} closure;

#define call(c, ...) ((void*(*)(closure*, ...))((closure*)c)->fn)(c, ##__VA_ARGS__)
#define build_closure(c, func, ...) \
    void *c = NULL; \
    { \
        c = malloc(sizeof(closure)); \
        ((closure*)c)->fn = func; \
        void *env[] = {__VA_ARGS__}; \
        ((closure*)c)->env = malloc(sizeof(env)); \
        memcpy(((closure*)c)->env, env, sizeof(env)); \
        ((closure*)c)->env_size = sizeof(env) / sizeof(void*); \
    }
#define build_tuple(t, ...) { \
    void *tuple[] = {__VA_ARGS__}; \
    t = malloc(sizeof(tuple)); \
    memcpy(t, tuple, sizeof(tuple)); \
}

/* Forward declare functions */
void func0(closure*, void*);

/* Forward Declare Classes */
typedef struct class0 class0;
void *new_class0(closure *c);
void init0(closure *c, class0 *var_self);
typedef struct class1 class1;
void *new_class1(closure *c);
void init1(closure *c, class1 *var_self);
typedef struct class2 class2;
void *new_class2(closure *c);
void init2(closure *c, class2 *var_self);
typedef struct class3 class3;
void *new_class3(closure *c);
void init3(closure *c, class3 *var_self);
typedef struct class4 class4;
void *new_class4(closure *c);
void init4(closure *c, class4 *var_self);
typedef struct class5 class5;
void *new_class5(closure *c);
void init5(closure *c, class5 *var_self);
typedef struct class6 class6;
void *new_class6(closure *c);
void init6(closure *c, class6 *var_self);

/* Define Classes */
struct class0 {
    int val;
    void *field_val;
    void *field_toString;
};
void *new_class0(closure *c) {
    class0 *var_self = malloc(sizeof(class0));
    init0(c, var_self);
    return var_self;
}
void *class0_toString(closure *c, class0 *self);
void init0(closure *c, class0 *var_self) {
    #define var_val var_self->field_val
    #define var_toString var_self->field_toString
    var_val = var_self;
    build_closure(tmp0, class0_toString);
    var_toString = tmp0;
    var_self->val = 0;
    #undef var_val
    #undef var_toString
}

struct class1 {
    double val;
    void *field_val;
    void *field_toString;
};
void *new_class1(closure *c) {
    class1 *var_self = malloc(sizeof(class1));
    init1(c, var_self);
    return var_self;
}
void *class1_toString(closure *c, class1 *self);
void init1(closure *c, class1 *var_self) {
    #define var_val var_self->field_val
    #define var_toString var_self->field_toString
    var_val = var_self;
    build_closure(tmp1, class1_toString);
    var_toString = tmp1;
    var_self->val = 0;
    #undef var_val
    #undef var_toString
}

struct class2 {
    char *val;
    void *field_val;
    void *field_toString;
};
void *new_class2(closure *c) {
    class2 *var_self = malloc(sizeof(class2));
    init2(c, var_self);
    return var_self;
}
void *class2_toString(closure *c, class2 *self);
void init2(closure *c, class2 *var_self) {
    #define var_val var_self->field_val
    #define var_toString var_self->field_toString
    var_val = var_self;
    build_closure(tmp2, class2_toString);
    var_toString = tmp2;
    var_self->val = 0;
    #undef var_val
    #undef var_toString
}

struct class3 {
    void *field_toString;
};
struct class4 {
    void *field_x;
};
void *new_class4(closure *c) {
    class4 *var_self = malloc(sizeof(class4));
    init4(c, var_self);
    return var_self;
}
void init4(closure *c, class4 *var_self) {
    #define var_x var_self->field_x
    #define var_double c->env[0]
    #define var_int c->env[1]
    #define var_println c->env[2]
    #define var_string c->env[3]
    void *tmp3 = call(var_int);
    ((class0*)tmp3)->val = 5;
    var_x = tmp3;
    #undef var_double
    #undef var_int
    #undef var_stringable
    #undef var_println
    #undef var_string
    #undef var_x
}

struct class5 {
    void *field_x;
    void *field_y;
};
void *new_class5(closure *c) {
    class5 *var_self = malloc(sizeof(class5));
    init5(c, var_self);
    return var_self;
}
void init5(closure *c, class5 *var_self) {
    #define var_x var_self->field_x
    #define var_y var_self->field_y
    #define var_double c->env[0]
    #define var_int c->env[1]
    #define var_A c->env[2]
    #define var_println c->env[3]
    #define var_string c->env[4]
    void *tmp4 = call(var_int);
    ((class0*)tmp4)->val = 10;
    var_x = tmp4;
    void *tmp5 = call(var_double);
    ((class1*)tmp5)->val = 4.000000;
    var_y = tmp5;
    #undef var_double
    #undef var_int
    #undef var_stringable
    #undef var_A
    #undef var_println
    #undef var_string
    #undef var_x
    #undef var_y
}

struct class6 {
    void *field_x;
};
void builtin_println(closure *c, void *val) {
    class3 *stringable = val;
    class2 *str = call(stringable->field_toString);
    printf("%s\n", str->val);
}

void *class0_toString(closure *c, class0 *self) {
    class2 *str = new_class2(NULL);
    int size = snprintf(NULL, 0, "%d", self->val);
    str->val = malloc(size + 1);
    sprintf(str->val, "%d", self->val);
    return str;
}
void *class1_toString(closure *c, class1 *self) {
    class2 *str = new_class2(NULL);
    int size = snprintf(NULL, 0, "%f", self->val);
    str->val = malloc(size + 1);
    sprintf(str->val, "%f", self->val);
    return str;
}
void *class2_toString(closure *c, class2 *self) {
    return self;
}
/* Define functions */
void func0(closure *c, void *arg) {
    void *var_t = arg;
    void *self;
    #define var_double c->env[0]
    #define var_int c->env[1]
    #define var_A c->env[2]
    #define var_B c->env[3]
    #define var_println c->env[4]
    #define var_string c->env[5]
    void *tmp6 = ((class6*)var_t)->field_x;
    void *tmp7 = call(var_println, tmp6);
    #undef var_double
    #undef var_int
    #undef var_stringable
    #undef var_has_x_Int
    #undef var_A
    #undef var_B
    #undef var_println
    #undef var_string
}

int main(int argc, char *argv[]) {
    void *var_double, *var_int, *var_A, *var_B, *var_b, *var_f, *var_println, *var_string;
    build_closure(tmp8, builtin_println)
    var_println = tmp8;
    build_closure(tmp9, new_class0)
    var_int = tmp9;
    build_closure(tmp10, new_class1)
    var_double = tmp10;
    build_closure(tmp11, new_class2)
    var_string = tmp11;
    build_closure(tmp12, new_class4, var_double, var_int, var_println, var_string)
    var_A = tmp12;
    build_closure(tmp13, new_class5, var_double, var_int, var_A, var_println, var_string)
    var_B = tmp13;
    build_closure(tmp14, func0, var_double, var_int, var_A, var_B, var_println, var_string)
    var_f = tmp14;
    void *tmp15 = call(var_B);
    var_b = tmp15;
    void *tmp16 = call(var_f, var_b);
}

