#ifndef VECTOR_H
#define VECTOR_H

#define VECTOR_CAPACITY 16

typedef struct vector Vector;

struct vector {
    void  *data;
    int   (*append)(const Vector *this, const void *val);
    int   (*get)   (const Vector *this, int index, const void *val_ptr);
    int   (*put)   (const Vector *this,
                    int index,
                    const void *val,
                    const void *prev_ptr);
    int   (*remove)(const Vector *this, int index, const void *prev_ptr);
    int   (*size)  (const Vector *this);
    int   (*array) (const Vector *this, int *size, const void *array_ptr);
    void  (*free)  (const Vector *this, void(*free_val)(void*));
    void  (*clear) (const Vector *this, void(*free_val)(void*));
    int   (*copy)  (const Vector *this,
                    const Vector **copy_ptr,
                    int (*val_copy)(const void*, const void*));
};

const Vector *new_Vector(int capacity);

#endif//VECTOR_H
