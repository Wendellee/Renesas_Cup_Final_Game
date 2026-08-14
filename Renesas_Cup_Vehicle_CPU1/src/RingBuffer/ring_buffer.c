#include "ring_buffer.h"

static uint16_t ring_buffer_next_index(uint16_t index)
{
    index++;
    return (index == RING_BUFFER_CAPACITY) ? 0U : index;
}

void RingBuffer_Init(ring_buffer_t * p_buffer)
{
    RingBuffer_Clear(p_buffer);
}

void RingBuffer_Clear(ring_buffer_t * p_buffer)
{
    p_buffer->write_index = 0U;
    p_buffer->read_index = 0U;
}

bool RingBuffer_Write(ring_buffer_t * p_buffer, uint8_t data)
{
    uint16_t next_index = ring_buffer_next_index(p_buffer->write_index);

    if (next_index == p_buffer->read_index)
    {
        return false;
    }

    p_buffer->storage[p_buffer->write_index] = data;
    p_buffer->write_index = next_index;
    return true;
}

bool RingBuffer_Read(ring_buffer_t * p_buffer, uint8_t * p_data)
{
    if ((NULL == p_data) || RingBuffer_IsEmpty(p_buffer))
    {
        return false;
    }

    *p_data = p_buffer->storage[p_buffer->read_index];
    p_buffer->read_index = ring_buffer_next_index(p_buffer->read_index);
    return true;
}

size_t RingBuffer_WriteBuffer(ring_buffer_t * p_buffer, const uint8_t * p_data, size_t length)
{
    size_t written = 0U;

    if (NULL == p_data)
    {
        return 0U;
    }
    while ((written < length) && RingBuffer_Write(p_buffer, p_data[written]))
    {
        written++;
    }
    return written;
}

size_t RingBuffer_ReadBuffer(ring_buffer_t * p_buffer, uint8_t * p_data, size_t length)
{
    size_t read = 0U;

    if (NULL == p_data)
    {
        return 0U;
    }
    while ((read < length) && RingBuffer_Read(p_buffer, &p_data[read]))
    {
        read++;
    }
    return read;
}

uint16_t RingBuffer_Count(const ring_buffer_t * p_buffer)
{
    uint16_t write_index = p_buffer->write_index;
    uint16_t read_index = p_buffer->read_index;

    if (write_index >= read_index)
    {
        return (uint16_t) (write_index - read_index);
    }

    return (uint16_t) (RING_BUFFER_CAPACITY - read_index + write_index);
}

uint16_t RingBuffer_Space(const ring_buffer_t * p_buffer)
{
    return (uint16_t) ((RING_BUFFER_CAPACITY - 1U) - RingBuffer_Count(p_buffer));
}

bool RingBuffer_IsEmpty(const ring_buffer_t * p_buffer)
{
    return (p_buffer->write_index == p_buffer->read_index);
}

bool RingBuffer_IsFull(const ring_buffer_t * p_buffer)
{
    return (ring_buffer_next_index(p_buffer->write_index) == p_buffer->read_index);
}
