#include "circular_queue.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>



/**
 * space:    [0, capacity]
 * data:     [bottom, top) or [bottom, capacity] + [0, top)
 * free:     [top, bottom) or [top, capacity] + [0, bottom)
 * top:      [0, capacity]
 * bottom:   [0, capacity]
 * 
 * 
 * 
 * 
 */
circular_queue_t* circular_queue_create(uint32_t elem_num_capacity, uint32_t elem_size)
{
    circular_queue_t* queue = (circular_queue_t*)malloc(sizeof(circular_queue_t) + (elem_num_capacity + 1) * elem_size);
    if (queue == NULL)
        return NULL;
    
    queue->capacity_logical = elem_num_capacity;
    queue->capacity_physical = elem_num_capacity + 1;
    queue->elem_size = elem_size;
    queue->bottom = 0;
    queue->top = 0;
    queue->buffer = (uint8_t*)queue + sizeof(circular_queue_t);
    
    return queue;
}

uint32_t circular_queue_get_elem_num(const circular_queue_t* ist)
{
    int32_t len = ist->top - ist->bottom;
    return len >= 0 ? len : len + ist->capacity_physical;
}

uint32_t circular_queue_is_empty(const circular_queue_t* ist)
{
    return ist->top == ist->bottom;
}

uint32_t circular_queue_is_full(const circular_queue_t* ist)
{
    return (ist->top + 1) % ist->capacity_logical == ist->bottom;
}

uint32_t circular_queue_enqueue(circular_queue_t* ist, uint8_t* data)
{
    if (circular_queue_is_full(ist))
        return CQ_DATA_OVERFLOW;
    
    memcpy(ist->buffer + (ist->elem_size * ist->top), data, ist->elem_size);    
    ist->top = ist->top == (ist->capacity_physical - 1) ? 0 : ist->top + 1;
    
    return CQ_OK;
}

uint32_t circular_queue_dequeue(circular_queue_t* ist, uint8_t* out_data)
{
    if (circular_queue_is_empty(ist))
        return CQ_DATA_OVERFLOW;
    
    memcpy(out_data, ist->buffer + (ist->elem_size * ist->bottom), ist->elem_size);
    ist->bottom = ist->bottom == (ist->capacity_physical - 1) ? 0 : ist->bottom + 1;
    
    return CQ_OK;
}

uint32_t circular_queue_enqueue_batch(circular_queue_t* ist, const uint8_t* elem_data, uint32_t elem_num)
{
    if (elem_num > ist->capacity_logical)
        return CQ_DATA_OVERFLOW;
    if (elem_num > ist->capacity_logical - circular_queue_get_elem_num(ist))
        return CQ_DATA_OVERFLOW;
    if (ist->top >= ist->bottom) {
        uint32_t part1_elem_num = elem_num;
        if (part1_elem_num > (ist->capacity_physical - ist->top))
            part1_elem_num = (ist->capacity_physical - ist->top);
        if (ist->top == 0)
            part1_elem_num = 0;
        uint32_t part2_elem_num = elem_num - part1_elem_num;
        
        
        if (part1_elem_num > 0)
            memcpy(ist->buffer + (ist->elem_size * ist->top), elem_data, (ist->elem_size * part1_elem_num));
        if (part2_elem_num > 0)
            memcpy(ist->buffer, elem_data + (ist->elem_size * part1_elem_num), (ist->elem_size * part2_elem_num));
        
        ist->top = part2_elem_num > 0 ? part2_elem_num : (ist->top + part1_elem_num) % ist->capacity_physical;
    } else {
        if (elem_num > 0)
            memcpy(ist->buffer + (ist->elem_size * ist->top), elem_data, (ist->elem_size * elem_num));
        
        ist->top = (ist->top + elem_num) % ist->capacity_physical;
    }
    return CQ_OK;
}

uint32_t circular_queue_dequeue_batch(circular_queue_t* ist, uint8_t* elem_data_out, uint32_t elem_num)
{
    if (elem_num > ist->capacity_logical)
        return CQ_DATA_OVERFLOW;
    if (elem_num > circular_queue_get_elem_num(ist))
        return CQ_DATA_OVERFLOW;
    
    if (ist-> top < ist->bottom) {
        uint32_t part1_elem_num = elem_num;
        if (part1_elem_num > (ist->capacity_physical - (ist->bottom)))
            part1_elem_num = (ist->capacity_physical - (ist->bottom));
        uint32_t part2_elem_num = elem_num - part1_elem_num;
        
        if (part1_elem_num > 0)
            memcpy(elem_data_out, ist->buffer + (ist->elem_size * ist->bottom), (ist->elem_size * part1_elem_num));
        if (part2_elem_num > 0)
            memcpy(elem_data_out + (ist->elem_size * part1_elem_num), ist->buffer, (ist->elem_size * part2_elem_num));
        
        ist->bottom = part2_elem_num > 0 ? part2_elem_num : (ist->bottom + part1_elem_num) % ist->capacity_physical;
    } else {
        if (elem_num > 0)
            memcpy(elem_data_out, ist->buffer + (ist->elem_size * ist->bottom), (ist->elem_size * elem_num));
        
        ist->bottom = (ist->bottom + elem_num) % ist->capacity_physical;
    }
    return CQ_OK;
}

uint32_t circular_queue_read_batch(const circular_queue_t* ist, uint8_t* elem_data_out, uint32_t elem_offset, uint32_t elem_num)
{
    uint32_t elem_total_num = elem_offset + elem_num;
    
    if (elem_total_num > ist->capacity_logical)
        return CQ_DATA_OVERFLOW;
    if (elem_total_num > circular_queue_get_elem_num(ist))
        return CQ_DATA_OVERFLOW;
    
    if (ist-> top < ist->bottom) {
        uint32_t part1_elem_num = elem_total_num;
        if (part1_elem_num > (ist->capacity_physical - (ist->bottom)))
            part1_elem_num = (ist->capacity_physical - (ist->bottom));
        uint32_t part2_elem_num = elem_total_num - part1_elem_num;
        
        if (part1_elem_num > 0)
            memcpy(elem_data_out, ist->buffer + (ist->elem_size * ist->bottom), (ist->elem_size * part1_elem_num));
        if (part2_elem_num > 0)
            memcpy(elem_data_out + (ist->elem_size * part1_elem_num), ist->buffer, (ist->elem_size * part2_elem_num));
    } else {
        if (elem_total_num > 0)
            memcpy(elem_data_out, ist->buffer + (ist->elem_size * ist->bottom), (ist->elem_size * elem_total_num));
    }
    
    memcpy(elem_data_out, elem_data_out + (ist->elem_size * elem_offset), (ist->elem_size * elem_num));
    
    return CQ_OK;
}

uint32_t circular_queue_discard_batch(circular_queue_t* ist, uint32_t elem_num)
{
    if (elem_num > ist->capacity_logical)
        return CQ_DATA_OVERFLOW;
    if (elem_num > circular_queue_get_elem_num(ist))
        return CQ_DATA_OVERFLOW;
    
    if (ist-> top < ist->bottom) {
        uint32_t part1_elem_num = elem_num;
        if (part1_elem_num > (ist->capacity_physical - (ist->bottom)))
            part1_elem_num = (ist->capacity_physical - (ist->bottom));
        uint32_t part2_elem_num = elem_num - part1_elem_num;
        
        ist->bottom = part2_elem_num > 0 ? part2_elem_num : (ist->bottom + part1_elem_num) % ist->capacity_physical;
    } else {
        ist->bottom = (ist->bottom + elem_num) % ist->capacity_physical;
    }
    
    return CQ_OK;
}

void circular_queue_destroy(circular_queue_t* ist)
{
    if (ist == NULL)
        return;
    
    free(ist);
}

#define massert(v) if (!(v)) return 1;

uint32_t circular_queue_test()
{
    circular_queue_t* queue = circular_queue_create(10, 1);
    
    uint8_t data[5] = { 0x00, 0x01, 0x02, 0x03, 0x04 };
    uint8_t data_result[5] = { 0x00 };
    
    for (uint32_t i = 0; i < 20; ++i) {
        circular_queue_enqueue_batch(queue, data, 5);
        massert(circular_queue_get_elem_num(queue) == 5);
        
        circular_queue_dequeue_batch(queue, data_result, 3);
        massert(circular_queue_get_elem_num(queue) == 2);
        massert(data_result[0] == 0x00);
        massert(data_result[1] == 0x01);
        massert(data_result[2] == 0x02);
        
        circular_queue_dequeue_batch(queue, data_result, 2);
        massert(circular_queue_get_elem_num(queue) == 0);
        massert(data_result[0] == 0x03);
        massert(data_result[1] == 0x04);
    }
    circular_queue_destroy(queue);
    return 0;
}

