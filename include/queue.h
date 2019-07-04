#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_CAPACITY 16

typedef struct queue Queue;

struct queue {
    void *data;
    int  (*push) (const Queue *this, const void *val);
    int  (*pop)  (const Queue *this, const void *val_ptr);
    int  (*front)(const Queue *this, const void *val_ptr);
    int  (*back) (const Queue *this, const void *val_ptr);
    int  (*size) (const Queue *this);
    void (*free) (const Queue *this, void (*free_val)(const void*));
};

const Queue *new_Queue(int capacity);

#endif//QUEUE_H
