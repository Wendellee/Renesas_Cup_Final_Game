#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Project-level configuration.
 * Change this one macro before compiling, or pass
 * -DRING_BUFFER_CAPACITY=<value> to every translation unit.
 */
#ifndef RING_BUFFER_CAPACITY
#define RING_BUFFER_CAPACITY    (512U)
#endif

#if (RING_BUFFER_CAPACITY < 2U)
#error "RING_BUFFER_CAPACITY must be at least 2."
#endif

/*
 * Byte-oriented single-producer/single-consumer ring buffer.
 * It is suitable for one ISR producer and one foreground consumer. One byte is
 * reserved internally to distinguish the full and empty states.
 */
typedef struct ring_buffer
{
    uint8_t           storage[RING_BUFFER_CAPACITY];
    volatile uint16_t write_index;
    volatile uint16_t read_index;
} ring_buffer_t;

void     RingBuffer_Init(ring_buffer_t * p_buffer);
void     RingBuffer_Clear(ring_buffer_t * p_buffer);
bool     RingBuffer_Write(ring_buffer_t * p_buffer, uint8_t data);
bool     RingBuffer_Read(ring_buffer_t * p_buffer, uint8_t * p_data);
size_t   RingBuffer_WriteBuffer(ring_buffer_t * p_buffer, const uint8_t * p_data, size_t length);
size_t   RingBuffer_ReadBuffer(ring_buffer_t * p_buffer, uint8_t * p_data, size_t length);
uint16_t RingBuffer_Count(const ring_buffer_t * p_buffer);
uint16_t RingBuffer_Space(const ring_buffer_t * p_buffer);
bool     RingBuffer_IsEmpty(const ring_buffer_t * p_buffer);
bool     RingBuffer_IsFull(const ring_buffer_t * p_buffer);

#endif
