#ifndef SAFE_H
#define SAFE_H

#define UNUSED __attribute__ ((unused))
#define RED     "\033[0;91m"
#define WHITE   "\033[0m"

#define print_ICE(msg) { \
        fprintf(stderr, \
            "%s:%d: " RED "internal compiler error: " WHITE "%s(): " msg "\n", \
            __FILE__, \
            __LINE__, \
            __func__); \
        exit(EXIT_FAILURE); \
    } \
    void print_ICE()

#define safe_method_call(obj, fn, ...) { \
        if ((obj)->fn(obj, __VA_ARGS__)) { \
            print_ICE(#obj "->" #fn "(" #__VA_ARGS__ ") failed"); \
        } \
    } \
    void safe_method_call()

#define safe_function_call(fn, ...) { \
        if (fn(__VA_ARGS__)) { \
            print_ICE(#fn "(" #__VA_ARGS__ ") failed"); \
        } \
    } \
    void safe_function_call()


#endif//SAFE_H
