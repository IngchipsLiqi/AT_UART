#ifndef __CIRCULAR_QUEUE_H__
#define __CIRCULAR_QUEUE_H__

#include "ingsoc.h"

#define CQ_RTOS_HEAP_OVERFLOW -1
#define CQ_DATA_OVERFLOW -2
#define CQ_OK 0

typedef struct {
    uint32_t capacity_logical;
    uint32_t capacity_physical;
    uint32_t elem_size;
    volatile uint32_t top;
    volatile uint32_t bottom;
    uint8_t* buffer;
} circular_queue_t;



circular_queue_t* circular_queue_create(uint32_t elem_num_capacity, uint32_t elem_size);
void circular_queue_destroy(circular_queue_t* ist);

uint32_t circular_queue_get_elem_num(const circular_queue_t* ist);

uint32_t circular_queue_is_empty(const circular_queue_t* ist);
uint32_t circular_queue_is_full(const circular_queue_t* ist);

uint32_t circular_queue_enqueue(circular_queue_t* ist, uint8_t* data);
uint32_t circular_queue_dequeue(circular_queue_t* ist, uint8_t* out_data);

uint32_t circular_queue_enqueue_batch(circular_queue_t* ist, const uint8_t* elem_data, uint32_t elem_num);
uint32_t circular_queue_dequeue_batch(circular_queue_t* ist, uint8_t* elem_data_out, uint32_t elem_num);

uint32_t circular_queue_read_batch(const circular_queue_t* ist, uint8_t* elem_data_out, uint32_t elem_offset, uint32_t elem_num);
uint32_t circular_queue_discard_batch(circular_queue_t* ist, uint32_t elem_num);

uint32_t circular_queue_test();

#endif