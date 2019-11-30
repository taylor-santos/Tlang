#include "map.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    unsigned long(*hash)(const void*, size_t);
    int (*keycmp)(const void*, size_t, const void*, size_t);
};

void print_Map(const Map *this, void (*printer)(const void *)) {
    Data *data = this->data;
    char *sep  = "";
    printf("{");
    for (size_t i = 0; i < data->capacity; i++) {
        for (Entry *curr = data->entries[i]; curr != NULL; curr = curr->next) {
            const void *val = curr->value;
            printf("%s", sep);
            printf("\"%.*s\": \"", (int) curr->len, (char *) curr->key);
            if (printer) printer(val);
            printf("\"");
            sep = ", ";
        }
    }
    printf("}");
}

Entry *new_Entry(
        const void *key, size_t len, const void *value, unsigned long hash
) {
    Entry *new = malloc(sizeof(*new));
    if (new == NULL) {
        return NULL;
    }
    new->next = NULL;
    new->key  = malloc(len);
    if (new->key == NULL) {
        free(new);
        return NULL;
    }
    memcpy((void *) new->key, key, len);
    new->len   = len;
    new->value = value;
    new->hash  = hash;
    return new;
}

int keycmp(const void *key1, size_t len1, const void *key2, size_t len2) {
    const char *k1 = key1, *k2 = key2;
    for (; len1 && len2; k1++, len1--, k2++, len2--) {
        if (*k1 != *k2) {
            return *k1 - *k2;
        }
    }
    if (len1 != len2) {
        return (int) len1 - (int) len2;
    }
    return 0;
}

unsigned long hash(const void *key, size_t len) {
    /* djb2 hash function
     * http://www.cse.yorku.ca/~oz/hash.html
     */
    unsigned long hash = 5381;
    size_t i;
    const char *k = key;
    for (i = 0; i < len; i++) {
        hash = ((hash << 5u) + hash) + *k++;
    }
    return hash;
}

int resize(const Map *this) {
    Data   *data         = this->data;
    size_t capacity      = data->capacity + data->cap_diff;
    Entry  **new_entries = calloc(capacity, sizeof(*new_entries));
    if (new_entries == NULL) {
        return 1;
    }
    for (size_t i = 0; i < data->capacity; i++) {
        for (Entry *old = data->entries[i]; old != NULL;) {
            unsigned long h = old->hash % capacity;
            Entry         **n;
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
    data->entries  = new_entries;
    data->capacity = capacity;
    return 0;
}

int map_put(
        const Map *this,
        const void *key,
        size_t len,
        const void *value,
        const void *prev_ptr
) {
    Data          *data = this->data;
    unsigned long h     = data->hash(key, len);
    Entry         **curr;
    for (curr = &data->entries[h % data->capacity];
         *curr != NULL;
         curr          = &(*curr)->next) {
        int cmp = data->keycmp(key, len, (*curr)->key, (*curr)->len);
        if (cmp == 0) {
            if (prev_ptr == NULL) {
                return 1;
            }
            *(const void **) prev_ptr = (*curr)->value;
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
            if ((double) data->size / data->capacity >= data->load_factor) {
                if (resize(this)) {
                    return 1;
                }
            }
            return 0;
        }
    }
    Entry         *new = new_Entry(key, len, value, h);
    if (new == NULL) {
        return 1;
    }
    *curr = new;
    data->size++;
    if ((double) data->size / data->capacity >= data->load_factor) {
        if (resize(this)) {
            return 1;
        }
    }
    return 0;
}

int map_get(
        const Map *this, const void *key, size_t len, const void *value_ptr
) {
    Data          *data = this->data;
    unsigned long h     = data->hash(key, len);
    Entry         *curr;
    for (curr = data->entries[h % data->capacity];
         curr != NULL;
         curr = curr->next) {
        int cmp = data->keycmp(key, len, curr->key, curr->len);
        if (cmp == 0) {
            if (value_ptr == NULL) {
                return 1;
            }
            *(const void **) value_ptr = curr->value;
            return 0;
        } else if (cmp < 0) {
            return 1;
        }
    }
    return 1;
}

int map_contains(const Map *this, const void *key, size_t len) {
    Data          *data = this->data;
    unsigned long h     = data->hash(key, len);
    Entry         *curr;
    for (curr = data->entries[h % data->capacity];
         curr != NULL;
         curr = curr->next) {
        int cmp = data->keycmp(key, len, curr->key, curr->len);
        if (cmp == 0) {
            return 1;
        } else if (cmp < 0) {
            return 0;
        }
    }
    return 0;
}

int map_copy(
        const Map *this,
        const Map **copy_ptr,
        int (*copy_val)(const void *, const void *)
) {
    if (copy_ptr == NULL) {
        return 1;
    }
    Data *data = this->data;
    *copy_ptr = new_Map(data->capacity, data->load_factor);
    Data *data_copy = (*copy_ptr)->data;
    data_copy->cap_diff = data->cap_diff;
    for (size_t i = 0; i < data->capacity; i++) {
        for (Entry *curr = data->entries[i]; curr != NULL; curr = curr->next) {
            const void *new = curr->value;
            if (copy_val != NULL) {
                if (copy_val(curr->value, &new)) {
                    return 1;
                }
            }
            if ((*copy_ptr)->put(*copy_ptr, curr->key, curr->len, new, NULL)) {
                return 1;
            }
        }
    }
    return 0;
}

int map_keys(
        const Map *this, size_t *count, size_t **lengths, const void *array_ptr
) {
    if (array_ptr == NULL || lengths == NULL || count == NULL) {
        return 1;
    }
    Data *data = this->data;
    *count   = data->size;
    if (data->size == 0) {
        *lengths = NULL;
        return 0;
    }
    *lengths = malloc(data->size * sizeof(**lengths));
    if (*lengths == NULL) {
        return 1;
    }
    void ***arr = (void ***) array_ptr;
    *arr = malloc(data->size * sizeof(*arr));
    if (*arr == NULL) {
        free(lengths);
        return 1;
    }
    size_t      curr_index = 0;
    for (size_t i          = 0; i < data->capacity; i++) {
        for (Entry *curr = data->entries[i]; curr != NULL; curr = curr->next) {
            (*lengths)[curr_index] = curr->len;
            (*arr)[curr_index]     = malloc(curr->len);
            memcpy((*arr)[curr_index], curr->key, curr->len);
            curr_index++;
        }
    }
    return 0;
}

size_t map_size(const Map *this) {
    Data *data = this->data;
    return data->size;
}

int map_set_hash(const Map *this,
                 unsigned long(*hashfn)(const void*, size_t),
                 int (*keycmpfn)(const void*, size_t, const void*, size_t)) {
    Data *data = this->data;
    Entry  **new_entries = calloc(data->capacity, sizeof(*new_entries));
    if (new_entries == NULL) {
        return 1;
    }
    for (size_t i = 0; i < data->capacity; i++) {
        for (Entry *old = data->entries[i]; old != NULL;) {
            unsigned long h = hashfn(old->key, old->len) % data->capacity;
            old->hash = h;
            Entry **n;
            for (n = &new_entries[h]; *n != NULL; n = &(*n)->next) {
                if (keycmpfn((old)->key, old->len, (*n)->key, (*n)->len) <= 0) {
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
    data->hash = hashfn;
    data->keycmp = keycmpfn;
    return 0;
}

void map_free(const Map *this, void (*val_free)(void *)) {
    Data        *data = this->data;
    for (size_t i     = 0; i < data->capacity; i++) {
        Entry *curr = data->entries[i];
        while (curr != NULL) {
            Entry *next = curr->next;
            if (val_free != NULL) {
                val_free((void *) curr->value);
            }
            free((void *) curr->key);
            free(curr);
            curr = next;
        }
    }
    free(data->entries);
    free(data);
    free((void *) this);
}

const Map *new_Map(size_t capacity, double load_factor) {
    Data *data = malloc(sizeof(*data));
    if (data == NULL) {
        return NULL;
    }
    data->size        = 0;
    data->capacity    = capacity <= 0
                        ? MAP_CAPACITY
                        : capacity;
    data->cap_diff    = capacity;
    data->load_factor = load_factor <= 0
                        ? MAP_LOAD_FACTOR
                        : load_factor;
    data->entries     = calloc(data->capacity, sizeof(*data->entries));
    if (data->entries == NULL) {
        free(data);
        return NULL;
    }
    data->hash = hash;
    data->keycmp = keycmp;
    Map *m = malloc(sizeof(*m));
    if (m == NULL) {
        free(data->entries);
        free(data);
        return NULL;
    }
    m->data     = data;
    m->put      = map_put;
    m->get      = map_get;
    m->contains = map_contains;
    m->copy     = map_copy;
    m->keys     = map_keys;
    m->size     = map_size;
    m->set_hash = map_set_hash;
    m->free     = map_free;
    return m;
}
