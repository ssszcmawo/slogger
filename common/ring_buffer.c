#include "ring_buffer.h"
#include <string.h>


void ring_init(ring_buffer_t* r, void *storage, uint32_t size)
{
    r->size = size;
    r->mask = size - 1;
    atomic_init(&r->head, 0);
    atomic_init(&r->tail, 0);
    r->data = (uint8_t*)storage;
}

uint32_t ring_count(const ring_buffer_t *r) {
    return atomic_load_explicit(&r->head, memory_order_acquire) -
           atomic_load_explicit(&r->tail, memory_order_acquire);
}

int ring_push(ring_buffer_t *r, const void *item, uint32_t item_size) {
    uint32_t h = atomic_load_explicit(&r->head, memory_order_relaxed);
    uint32_t t = atomic_load_explicit(&r->tail, memory_order_acquire);

    if (h - t == r->size)
        return 0;

    memcpy(r->data + (h & r->mask) * item_size, item, item_size);
    atomic_store_explicit(&r->head, h + 1, memory_order_release);
    return 1;
}

int ring_pop(ring_buffer_t *r, void *item, uint32_t item_size) {
    uint32_t t = atomic_load_explicit(&r->tail, memory_order_relaxed);
    uint32_t h = atomic_load_explicit(&r->head, memory_order_acquire);

    if (h == t)
        return 0;

    memcpy(item, r->data + (t & r->mask) * item_size, item_size);
    atomic_store_explicit(&r->tail, t + 1, memory_order_release);
    return 1;
}


