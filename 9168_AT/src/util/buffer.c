#include "buffer.h"




//==============================================================================================================
// * mBuffer
//==============================================================================================================

void mbuffer_init(mbuffer_t* instance)
{
    memset(instance, 0, sizeof(mbuffer_t));
    
    instance->send_buffer_operation_mutex = xSemaphoreCreateMutex();
}


void mbuffer_lock(mbuffer_t* ist)
{
    xSemaphoreTake(ist->send_buffer_operation_mutex, portMAX_DELAY);
}

void mbuffer_unlock(mbuffer_t* ist)
{
    xSemaphoreGive(ist->send_buffer_operation_mutex);
}

uint32_t mbuffer_store_data(mbuffer_t* ist, uint8_t* buf, uint32_t size) 
{
    uint32_t remaining = MBUFFER_REMAINING(ist);
    if (remaining < size)
    {
        return RETCODE_SEND_BUFFER_IS_NOT_ENOUGH;
    }
    
    memcpy(ist->buffer + ist->limit, buf, size);
    ist->limit += size;
    
    return RETCODE_OK;
}

uint32_t mbuffer_store_data_sync(mbuffer_t* ist, uint8_t* buf, uint32_t size) 
{
    mbuffer_lock(ist);
    uint32_t r = mbuffer_store_data(ist, buf, size);
    mbuffer_unlock(ist);
    return r;
}

void mbuffer_compact(mbuffer_t* ist) 
{
    uint32_t validDataSize = MBUFFER_VALIDDATA(ist);
    memcpy(ist->buffer, ist->buffer + ist->position, validDataSize);
    ist->position = 0;
    ist->limit = validDataSize;
}

uint32_t mbuffer_release_data(mbuffer_t* ist, uint8_t* out_buf, uint32_t size)
{
    uint32_t validData = MBUFFER_VALIDDATA(ist);
    if (validData < size)
    {
        return RETCODE_SEND_BUFFER_IS_NOT_ENOUGH;
    }
    
    memcpy(out_buf, ist->buffer + ist->position, size);
    ist->position += size;
    
    mbuffer_compact(ist);
    
    return RETCODE_OK;
}

uint32_t mbuffer_release_data_sync(mbuffer_t* ist, uint8_t* out_buf, uint32_t size) 
{
    mbuffer_lock(ist);
    uint32_t r = mbuffer_release_data(ist, out_buf, size);
    mbuffer_unlock(ist);
    return r;
}

uint32_t mbuffer_try_release_data_direct(mbuffer_t* ist, uint32_t size, try_release_data_t try_release_data)
{
    uint32_t validData = MBUFFER_VALIDDATA(ist);
    if (validData < size)
    {
        return RETCODE_SEND_BUFFER_IS_NOT_ENOUGH;
    }
    
    uint32_t is_released = try_release_data(ist->buffer + ist->position, size);
    if (!is_released)
    {
        return RETCODE_RELEASE_DATA_FAIL;
    }
    
    ist->position += size;
    mbuffer_compact(ist);
    
    return RETCODE_OK;
}

uint32_t mbuffer_try_release_data_direct_sync(mbuffer_t* ist, uint32_t size, try_release_data_t try_release_data) 
{
    mbuffer_lock(ist);
    uint32_t r = mbuffer_try_release_data_direct(ist, size, try_release_data);
    mbuffer_unlock(ist);
    return r;
}

uint32_t mbuffer_clear(mbuffer_t* ist)
{
    ist->position = 0;
    ist->limit = 0;
    return 0;
}

uint32_t mbuffer_clear_sync(mbuffer_t* ist)
{
    mbuffer_lock(ist);
    uint32_t r = mbuffer_clear(ist);
    mbuffer_unlock(ist);
    return r;
}
//==============================================================================================================

