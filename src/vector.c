#include <stdlib.h>
#include <string.h> // memcpy()
#include <stdio.h>
#include "vector.h"
#include "safe.h"

struct vector_data {
    const void **values;
    size_t capacity;
    size_t cap_diff;
    size_t size;
};

static int vector_append(const Vector *this, const void *val) {
    struct vector_data *data = this->data;
    if (data->size == data->capacity) {
        size_t new_cap = data->capacity + data->cap_diff;
        const void **new_values =
            realloc(data->values, new_cap * sizeof(const void*));
        if (new_values == NULL) {
            return 1;
        }
        data->values = new_values;
        data->capacity = new_cap;
    }
    data->values[data->size++] = val;
    return 0;
}

static int vector_get(const Vector *this, size_t index, const void *val_ptr) {
    struct vector_data *data = this->data;
    if (index >= data->size) {
        return 1;
    }
    if (val_ptr == NULL) {
        return 1;
    }
    *(const void**)val_ptr = data->values[index];
    return 0;
}

static int vector_put(const Vector *this, size_t index, const void *element,
                      const void *prev_ptr) {
    struct vector_data *data = this->data;
    if (index >= data->size) {
        return 1;
    }
    if (prev_ptr != NULL) {
        *(const void**)prev_ptr = data->values[index];
    }
    data->values[index] = element;
    return 0;
}

static int vector_remove(const Vector *this,
                         size_t index,
                         const void *prev_ptr) {
    struct vector_data *data = this->data;
    if (index >= data->size) {
        return 1;
    }
    if (prev_ptr != NULL) {
        *(const void**)prev_ptr = data->values[index];
    }
    for (size_t i = index; i < data->size - 1; i++) {
        data->values[i] = data->values[i + 1];
    }
    data->size--;
    return 0;
}

static size_t vector_size(const Vector *this) {
    struct vector_data *data = this->data;
    return data->size;
}

static int vector_array(const Vector *this,
                        size_t *size,
                        const void *array_ptr) {
    if (size == NULL) {
        return 1;
    }
    if (array_ptr == NULL) {
        return 1;
    }
    struct vector_data *data = this->data;
    if (data->size == 0) {
        *size = 0;
        *(const void**)array_ptr = NULL;
        return 0;
    }
    void *values = malloc(data->size * sizeof(const void*));
    if (values == NULL) {
        return 1;
    }
    memcpy(values, data->values, data->size * sizeof(const void*));
    *size = data->size;
    *(const void***)array_ptr = values; // dereference ptr to array of ptrs
    return 0;
}

static void vector_free(const Vector *this, void (*free_val)(void*)) {
    struct vector_data *data = this->data;
    if (free_val != NULL) {
        for (size_t i = 0; i < data->size; i++) {
            free_val((void*)data->values[i]);
        }
    }
    free(data->values);
    free(data);
    free((void*)this);
}

static void vector_clear(const Vector *this, void (*free_val)(void*)) {
    struct vector_data *data = this->data;
    if (free_val != NULL) {
        for (size_t i = 0; i < data->size; i++) {
            free_val((void*)data->values[i]);
        }
    }
    data->size = 0;
}

static int vector_copy(const Vector *this,
                       const Vector **copy_ptr,
                       int (*val_copy)(const void*, const void*)) {
    if (copy_ptr == NULL) {
        return 1;
    }
    struct vector_data *data = this->data;
    *copy_ptr = new_Vector(data->capacity);
    struct vector_data *copy_data = (*copy_ptr)->data;
    copy_data->cap_diff = data->cap_diff;
    for (size_t i = 0; i < data->size; i++) {
        const void *new_val = NULL;
        if (val_copy != NULL) {
            safe_function_call(val_copy, data->values[i], &new_val);
        } else {
            new_val = data->values[i];
        }
        if ((*copy_ptr)->append(*copy_ptr, new_val)) {
            return 1;
        }
    }
    return 0;
}

const Vector *new_Vector(size_t capacity) {
    struct vector_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        return NULL;
    }
    size_t cap = capacity > 0 ? capacity : VECTOR_CAPACITY;
    data->capacity = data->cap_diff = cap;
    data->values = malloc(cap * sizeof(const void*));
    if (data->values == NULL) {
        free(data);
        return NULL;
    }
    data->size = 0;
    Vector *vector = malloc(sizeof(*vector));
    if (vector == NULL) {
        free(data->values);
        free(data);
        return NULL;
    }
    vector->data   = data;
    vector->append = vector_append;
    vector->get    = vector_get;
    vector->put    = vector_put;
    vector->remove = vector_remove;
    vector->size   = vector_size;
    vector->array  = vector_array;
    vector->free   = vector_free;
    vector->clear  = vector_clear;
    vector->copy   = vector_copy;
    return vector;
}
