#ifndef STACK_H
#define STACK_H

#define DEFAULT_CAPACITY 16

typedef struct stack Stack;

struct stack {
    void *data;
    int  (*push)(const Stack*, void*);
    int  (*pop) (const Stack*, void**);
    int  (*top) (const Stack*, void**);
    int  (*size)(const Stack*);
    void (*free)(const Stack*, void (*)(void*));
};

const Stack *new_Stack(int cap);

#endif//STACK_H
