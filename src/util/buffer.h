#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "../common/preh.h"

#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"

//==============================================================================================================
// * mBuffer
//==============================================================================================================
#define RETCODE_OK 0
#define RETCODE_SEND_BUFFER_IS_NOT_ENOUGH -1
#define RETCODE_ERROR -2
#define RETCODE_RELEASE_DATA_FAIL -3

#define MBUFFER_CAPACITY 5000

#define MBUFFER_REMAINING(buf) (MBUFFER_CAPACITY - (buf)->limit   )
#define MBUFFER_VALIDDATA(buf) (    (buf)->limit - (buf)->position)

typedef uint32_t (*try_release_data_t)(uint8_t* buf, uint32_t size);

/**
 * From `position` to `limit` is a valid data area
 * Reading data can only start at `send_buffer_position` and end at `send_buffer_limit`
 * 
 * From `limit` to `MBUFFER_CAPACITY` is the remaining free area
 * Storing data can only start at `limit` and end at `MBUFFER_CAPACITY`
 * 
 * Follow the left-closed-right-open principle
 * position <= limit <= MBUFFER_CAPACITY
 * 
 */
struct mbuffer_t
{
    uint32_t limit;
    uint32_t position;
    uint8_t buffer[MBUFFER_CAPACITY];
    SemaphoreHandle_t send_buffer_operation_mutex;
};
typedef struct mbuffer_t mbuffer_t;

/**
 * init
 * 
 */
void mbuffer_init(mbuffer_t* instance);

/**
 * Store data into the `mbuffer`
 * 
 * If the buffer remaining free area is less than the specified size, RETCODE_SEND_BUFFER_IS_NOT_ENOUGH is returned
 * 
 * The heap memory allocator is not used and the buffer cannot be dynamically expanded
 * 
 */
uint32_t mbuffer_store_data_sync(mbuffer_t* ist, uint8_t* buf, uint32_t size);

/**
 * Make `mbuffer` data compact
 * 
 * Move the data from the[position, limit) range 
 *                 to the[0, limit - position) range
 * 
 */
void mbuffer_compact(mbuffer_t* ist);

/**
 * Release data from the `mbuffer` to the `out_buf`
 * 
 * Then compact the `mbuffer`
 * 
 * If the buffer valid data size is less than the specified size, RETCODE_SEND_BUFFER_IS_NOT_ENOUGH is returned
 * 
 */
uint32_t mbuffer_release_data_sync(mbuffer_t* ist, uint8_t* out_buf, uint32_t size);

/**
 * Release data from the `mbuffer`
 * 
 * If the try_release returns OK, the buffered data is released, otherwise it will remain unchanged
 * 
 * You can process the read data in the try_release_data()
 * 
 */
uint32_t mbuffer_try_release_data_direct_sync(mbuffer_t* ist, uint32_t size, try_release_data_t try_release_data);


/**
 * Clear data
 * 
 */
uint32_t mbuffer_clear_sync(mbuffer_t* ist);




void mbuffer_lock(mbuffer_t* ist);
void mbuffer_unlock(mbuffer_t* ist);
//==============================================================================================================


#endif