#include "stack.h"
#include <stdlib.h>
#include <stdio.h>

struct stack_data {
    void **values;
    int capacity;
    int cap_diff;
    int size;
};

static int stack_push(const Stack *this, void *element) {
    struct stack_data *data = this->data;
    if (data->size == data->capacity) {
        size_t new_cap = data->capacity + data->cap_diff;
        void **new_values = realloc(data->values, new_cap * sizeof(void*));
        if (new_values == NULL) {
            return 1;
        }
        data->values = new_values;
        data->capacity = new_cap;
    }
    data->values[data->size++] = element;
    return 0;
}

static int stack_pop(const Stack *this, void **element_ptr) {
    struct stack_data *data = this->data;
    if (element_ptr == NULL) {
        return 1;
    }
    if (data->size <= 0) {
        *element_ptr = NULL;
        return 1;
    }
    *element_ptr = data->values[--data->size];
    return 0;
}

static int stack_top(const Stack *this, void **element_ptr) {
    struct stack_data *data = this->data;
    if (element_ptr == NULL) {
        return 1;
    }
    if (data->size <= 0) {
        *element_ptr = NULL;
        return 1;
    }
    *element_ptr = data->values[data->size - 1];
    return 0;
}

static int stack_size(const Stack *this) {
    struct stack_data *data = this->data;
    return data->size;
}

static void stack_free(const Stack *this, void (*data_free)(void*)) {
    struct stack_data *data = this->data;
    if (data_free != NULL) {
        for (int i = 0; i < data->size; i++) {
            data_free(data->values[i]);
        }
    }
    free(data->values);
    free(data);
    free((void*)this);
}

const Stack *new_Stack(int cap) {
    struct stack_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        return NULL;
    }
    int capacity = cap > 0 ? cap : DEFAULT_CAPACITY;
    data->capacity = data->cap_diff = capacity;
    data->values = malloc(capacity * sizeof(void*));
    if (data->values == NULL) {
        free(data);
        return NULL;
    }
    data->size = 0;
    Stack *stack = malloc(sizeof(*stack));
    if (stack == NULL) {
        free(data->values);
        free(data);
        return NULL;
    }
    stack->data = data;
    stack->push = stack_push;
    stack->pop  = stack_pop;
    stack->top  = stack_top;
    stack->size = stack_size;
    stack->free = stack_free;
    return stack;
}