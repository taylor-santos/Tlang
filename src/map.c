#include "map.h"
#include <stdlib.h>
#include <string.h>

typedef struct entry Entry;
typedef struct data  Data;

struct entry {
    Entry         *next;
    const void    *key;
    size_t        len;
    const void    *value;
    unsigned long hash;
};

struct data {
    size_t size;
    size_t capacity;
    size_t cap_diff;
    double load_factor;
    Entry  **entries;
};

Entry *new_Entry(const void *key,
                 size_t len,
                 const void *value,
                 unsigned long hash) {
    Entry *new = malloc(sizeof(*new));
    if (new == NULL) {
        return NULL;
    }
    new->next = NULL;
    new->key = malloc(len);
    if (new->key == NULL) {
        free(new);
        return NULL;
    }
    memcpy((void*)new->key, key, len);
    new->len = len;
    new->value = value;
    new->hash = hash;
    return new;
}

unsigned long hash(const char *key, size_t len) {
    /* djb2 hash function
     * http://www.cse.yorku.ca/~oz/hash.html
     */
    unsigned long hash = 5381;
    size_t i;
    for (i = 0; i < len; i++) {
        hash = ((hash << 5u) + hash) + *key++;
    }
    return hash;
}

int keycmp(const char *key1, size_t len1, const char *key2, size_t len2) {
    for (; len1 && len2; key1++, len1--, key2++, len2--) {
        if (*key1 != *key2) { return *key1 - *key2; }
    }
    if (len1 != len2) {
        return (int)len1 - (int)len2;
    }
    return 0;
}

int resize(const Map *this) {
    Data *data = this->data;
    size_t capacity = data->capacity + data->cap_diff;
    Entry **new_entries = calloc(capacity, sizeof(*new_entries));
    if (new_entries == NULL) {
        return 1;
    }
    for (size_t i = 0; i < data->capacity; i++) {
        for (Entry *old = data->entries[i]; old != NULL; ) {
            unsigned long h = old->hash % capacity;
            Entry **n;
            for (n = &new_entries[h]; *n != NULL; n = &(*n)->next) {
                if (keycmp(old->key, old->len, (*n)->key, (*n)->len) <= 0) {
                    break;
                }
            }
            Entry *tmp = old->next;
            old->next = *n;
            *n = old;
            old = tmp;
        }
    }
    free(data->entries);
    data->entries = new_entries;
    data->capacity = capacity;
    return 0;
}

int map_put(const Map *this,
            const void *key,
            size_t len,
            const void *value,
            const void *prev_ptr) {
    Data *data = this->data;
    int h = hash(key, len);
    Entry **curr;
    for (curr = &data->entries[h % data->capacity];
         *curr != NULL;
         curr = &(*curr)->next) {
        int cmp = keycmp(key, len, (*curr)->key, (*curr)->len);
        if (cmp == 0) {
            if (prev_ptr == NULL) {
                return 1;
            }
            *(const void**)prev_ptr = (*curr)->value;
            (*curr)->value = value;
            return 0;
        } else if (cmp < 0) {
            Entry *new = new_Entry(key, len, value, h);
            if (new == NULL) {
                return 1;
            }
            new->next = *curr;
            *curr = new;
            data->size++;
            if ((double)data->size / data->capacity >= data->load_factor) {
                if (resize(this)) {
                    return 1;
                }
            }
            return 0;
        }
    }
    Entry *new = new_Entry(key, len, value, h);
    if (new == NULL) {
        return 1;
    }
    *curr = new;
    data->size++;
    if ((double)data->size / data->capacity >= data->load_factor) {
        if (resize(this)) {
            return 1;
        }
    }
    return 0;
}

int map_get(const Map *this,
            const void *key,
            size_t len,
            const void *value_ptr) {
    Data *data = this->data;
    int h = hash(key, len);
    Entry *curr;
    for (curr = data->entries[h % data->capacity];
         curr != NULL;
         curr = curr->next) {
        int cmp = keycmp(key, len, curr->key, curr->len);
        if (cmp == 0) {
            if (value_ptr == NULL) {
                return 1;
            }
            *(const void**)value_ptr = curr->value;
            return 0;
        } else if (cmp < 0) {
            return 1;
        }
    }
    return 1;
}

int map_contains(const Map *this, const void *key, size_t len) {
    Data *data = this->data;
    int h = hash(key, len);
    Entry *curr;
    for (curr = data->entries[h % data->capacity];
         curr != NULL;
         curr = curr->next) {
        int cmp = keycmp(key, len, curr->key, curr->len);
        if (cmp == 0) {
            return 1;
        } else if (cmp < 0) {
            return 0;
        }
    }
    return 0;
}

void map_free(const Map *this, void (*val_free)(void*)) {
    Data *data = this->data;
    for (size_t i = 0; i < data->capacity; i++) {
        Entry *curr = data->entries[i];
        while (curr != NULL) {
            Entry *next = curr->next;
            if (val_free != NULL) {
                val_free((void*)curr->value);
            }
            free((void*)curr->key);
            free(curr);
            curr = next;
        }
    }
    free(data->entries);
    free(data);
    free((void*)this);
}

const Map *new_Map(size_t capacity, double load_factor) {
    Data *data = malloc(sizeof(*data));
    if (data == NULL) {
        return NULL;
    }
    data->size = 0;
    data->capacity    = capacity    <= 0 ? MAP_CAPACITY    : capacity;
    data->cap_diff = capacity;
    data->load_factor = load_factor <= 0 ? MAP_LOAD_FACTOR : load_factor;
    data->entries = calloc(data->capacity, sizeof(*data->entries));
    if (data->entries == NULL) {
        free(data);
        return NULL;
    }
    Map *m = malloc(sizeof(*m));
    if (m == NULL) {
        free(data->entries);
        free(data);
        return NULL;
    }
    m->data = data;
    m->put      = map_put;
    m->get      = map_get;
    m->contains = map_contains;
    m->free     = map_free;
    return m;

}
