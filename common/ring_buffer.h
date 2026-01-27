#pragma once
#include <stdatomic.h>
#include <stdint.h>

typedef struct
{
    uint32_t size;
    uint32_t mask;
    _Atomic uint32_t head __attribute__((aligned(64)));
    _Atomic uint32_t tail __attribute__((aligned(64)));
    uint8_t *data;
} ring_buffer_t;

void ring_init(ring_buffer_t *r, void *storage, uint32_t size);
uint32_t ring_count(const ring_buffer_t *r);
int ring_push(ring_buffer_t *r, const void *item, uint32_t item_size);
int ring_pop(ring_buffer_t *r, void *item, uint32_t item_size);
