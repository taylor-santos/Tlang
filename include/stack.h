#ifndef STACK_H
#define STACK_H

#define STACK_CAPACITY 16

typedef struct stack Stack;

struct stack {
    void *data;
    int  (*push)(const Stack *this, const void *val);
    int  (*pop) (const Stack *this, const void *val_ptr);
    int  (*top) (const Stack *this, const void *val_ptr);
    int  (*size)(const Stack *this);
    void (*free)(const Stack *this, void (*free_val)(const void*));
};

const Stack *new_Stack(int capacity);

#endif//STACK_H
