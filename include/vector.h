#ifndef VECTOR_H
#define VECTOR_H

#define VECTOR_CAPACITY 16

typedef struct vector Vector;

struct vector {
    void   *data;
    int    (*append)(const Vector *this, const void *val);
    int    (*get)   (const Vector *this, size_t index, const void *val_ptr);
    int    (*put)   (const Vector *this,
                     size_t index,
                     const void *val,
                     const void *prev_ptr);
    int    (*remove)(const Vector *this, size_t index, const void *prev_ptr);
    size_t (*size)  (const Vector *this);
    int    (*array) (const Vector *this, size_t *size, const void *array_ptr);
    void   (*free)  (const Vector *this, void(*free_val)(void*));
    void   (*clear) (const Vector *this, void(*free_val)(void*));
    int    (*copy)  (const Vector *this,
                     const Vector **copy_ptr,
                     int (*val_copy)(const void*, const void*));
    void   (*sort)  (const Vector *this, int (*comp)(const void *, const void *));
};

const Vector *new_Vector(size_t capacity);

#endif//VECTOR_H
