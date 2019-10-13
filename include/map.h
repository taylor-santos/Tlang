#ifndef MAP_H
#define MAP_H

#include <stddef.h> // size_t

typedef struct map Map;

#define MAP_CAPACITY 16
#define MAP_LOAD_FACTOR 0.75

struct map {
    void   *data;
    int    (*put)     (const Map*, const void*, size_t, const void*, const void*);
    int    (*get)     (const Map*, const void*, size_t, const void*);
    int    (*contains)(const Map*, const void*, size_t);
    int    (*copy)    (const Map*, const Map**, int(*)(const void*, const void*));
    int    (*keys)    (const Map*, int*, size_t**, const void*);
    size_t (*size)    (const Map*);
    void   (*free)    (const Map*, void (*)(void*));
};

const Map *new_Map(size_t, double);

void print_Map(const Map*, void(*printer)(const void*));

#endif//MAP_H
