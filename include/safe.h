#ifndef SAFE_H
#define SAFE_H

#include <stdarg.h>
#include <stdlib.h>

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

#define safe_fprintf(out, ...) { \
        if (fprintf(out, __VA_ARGS__) < 0) { \
            print_ICE("file printing failed"); \
            fclose(out); \
            exit(EXIT_FAILURE); \
        } \
    } \
    void safe_fprintf()

#define safe_vfprintf(out, fmt, args) { \
        if (vfprintf(out, fmt, args) < 0) { \
            print_ICE("file printing failed"); \
            fclose(out); \
            exit(EXIT_FAILURE); \
        } \
    } \
    void safe_vfprintf()

int asprintf(char **strp, const char *fmt, ...);

#define safe_asprintf(strp, fmt, ...) { \
        if (asprintf(strp, fmt, __VA_ARGS__) < 0) { \
            print_ICE("file printing failed"); \
            exit(EXIT_FAILURE); \
        } \
    } \
    void safe_asprintf()

#endif//SAFE_H
