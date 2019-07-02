#include "queue.h"
#include <stdlib.h>

struct queue_data {
    const void **values;
    int capacity;
    int cap_diff;
    int size;
    int front;
    int back;
};

static int queue_push(const Queue *this, const void *val) {
    struct queue_data *data = this->data;
    if (data->size + 1 == data->capacity) {
        size_t new_cap = data->capacity + data->cap_diff;
        const void **new_values = realloc(data->values,
                                          new_cap * sizeof(const void*));
        if (new_values == NULL) {
            return 1;
        }
        data->values = new_values;
        if (data->back < data->front) {
            for (int i = 0; i < data->back; i++) {
                data->values[(data->capacity + i) % new_cap] = data->values[i];
            }
            data->back = (data->capacity + data->back) % new_cap;
        }
        data->values = new_values;
        data->capacity = new_cap;
    }
    data->values[data->back] = val;
    data->size++;
    data->back = (data->back + 1) % data->capacity;
    return 0;
}

static int queue_pop(const Queue *this, const void *val_ptr) {
    struct queue_data *data = this->data;
    if (val_ptr == NULL) {
        return 1;
    }
    if (data->size <= 0) {
        *(const void**)val_ptr = NULL;
        return 1;
    }
    *(const void**)val_ptr = data->values[data->front];
    data->front = (data->front + 1) % data->capacity;
    data->size--;
    return 0;
}

static int queue_front(const Queue *this, const void *val_ptr) {
    struct queue_data *data = this->data;
    if (val_ptr == NULL) {
        return 1;
    }
    if (data->size <= 0) {
        *(const void**)val_ptr = NULL;
        return 1;
    }
    *(const void**)val_ptr = data->values[data->front];
    return 0;
}

static int queue_size(const Queue *this) {
    struct queue_data *data = this->data;
    return data->size;
}

static void queue_free(const Queue *this, void (*val_free)(const void*)) {
    struct queue_data *data = this->data;
    if (val_free != NULL) {
        for (int i = 0; i < data->size; i++) {
            val_free(data->values[(data->front + i) % data->capacity]);
        }
    }
    free(data->values);
    free(data);
    free((void*)this);
}

const Queue *new_Queue(int capacity) {
    struct queue_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        return NULL;
    }
    int cap = capacity > 0 ? capacity : QUEUE_CAPACITY;
    data->capacity = data->cap_diff = cap;
    data->values = malloc(cap * sizeof(const void*));
    if (data->values == NULL) {
        free(data);
        return NULL;
    }
    data->size = 0;
    data->front = 0;
    data->back = 0;
    Queue *queue = malloc(sizeof(*queue));
    if (queue == NULL) {
        free(data->values);
        free(data);
        return NULL;
    }
    queue->data  = data;
    queue->push  = queue_push;
    queue->pop   = queue_pop;
    queue->front = queue_front;
    queue->size  = queue_size;
    queue->free  = queue_free;
    return queue;
}

