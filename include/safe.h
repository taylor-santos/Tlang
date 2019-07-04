#ifndef SAFE_H
#define SAFE_H

#define UNUSED __attribute__ ((unused))
#define RED     "\033[0;91m"
#define WHITE   "\033[0m"
#define safe_method_call(obj, fn, ...) { \
        if ((obj)->fn(obj, __VA_ARGS__)) { \
            fprintf(stderr, \
                "%s:%d: " RED "internal compiler error: " WHITE #obj "." \
                 #fn "(" #__VA_ARGS__ ") failed\n", \
                __FILE__, \
                __LINE__); \
            exit(EXIT_FAILURE); \
        } \
    } \
    void safe_method_call()

#define safe_function_call(fn, ...) { \
        if (fn(__VA_ARGS__)) { \
            fprintf(stderr, \
                "%s:%d: " RED "internal compiler error: " WHITE \
                 #fn "(" #__VA_ARGS__ ") failed\n", \
                __FILE__, \
                __LINE__); \
            exit(EXIT_FAILURE); \
        } \
    } \
    void safe_function_call()

#endif//SAFE_H
