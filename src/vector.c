#include "vector.h"
#include <stdlib.h>
#include <string.h> // memcpy()
#include <stdio.h>

struct vector_data {
    const void **values;
    int capacity;
    int cap_diff;
    int size;
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

static int vector_get(const Vector *this, int index, const void *val_ptr) {
    struct vector_data *data = this->data;
    if (index < 0 || index >= data->size) {
        return 1;
    }
    if (val_ptr == NULL) {
        return 1;
    }
    *(const void**)val_ptr = data->values[index];
    return 0;
}

static int vector_put(const Vector *this, int index, const void *element,
                      const void *prev_ptr) {
    struct vector_data *data = this->data;
    if (index < 0 || index >= data->size) {
        return 1;
    }
    if (prev_ptr != NULL) {
        *(const void**)prev_ptr = data->values[index];
    }
    data->values[index] = element;
    return 0;
}

static int vector_remove(const Vector *this, int index, const void *prev_ptr) {
    struct vector_data *data = this->data;
    if (index < 0 || index >= data->size) {
        return 1;
    }
    if (prev_ptr != NULL) {
        *(const void**)prev_ptr = data->values[index];
    }
    for (int i = index; i < data->size - 1; i++) {
        data->values[i] = data->values[i + 1];
    }
    data->size--;
    return 0;
}

static int vector_size(const Vector *this) {
    struct vector_data *data = this->data;
    return data->size;
}

static void **vector_array(const Vector *this, int *size) {
    if (size == NULL) {
        return NULL;
    }
    struct vector_data *data = this->data;
    void **values = malloc(data->size * sizeof(const void*));
    if (values == NULL) {
        return NULL;
    }
    memcpy(values, data->values, data->size * sizeof(const void*));
    *size = data->size;
    return values;
}

static void vector_free(const Vector *this, void (*free_val)(const void*)) {
    struct vector_data *data = this->data;
    if (free_val != NULL) {
        for (int i = 0; i < data->size; i++) {
            free_val((void*)data->values[i]);
        }
    }
    free(data->values);
    free(data);
    free((void*)this);
}

const Vector *new_Vector(int capacity) {
    struct vector_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        return NULL;
    }
    int cap = capacity > 0 ? capacity : VECTOR_CAPACITY;
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
    return vector;
}
