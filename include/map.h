#ifndef MAP_H
#define MAP_H

#include <stddef.h> // size_t

typedef struct map Map;

#define MAP_CAPACITY 16
#define MAP_LOAD_FACTOR 0.75

struct map {
    void *data;
    int  (*put)     (const Map*, const void*, size_t, const void*, const void*);
    int  (*get)     (const Map*, const void*, size_t, const void*);
    int  (*contains)(const Map*, const void*, size_t);
    void (*free)    (const Map*, void (*)(void*));
};

const Map *new_Map(size_t, double);

#endif//MAP_H
