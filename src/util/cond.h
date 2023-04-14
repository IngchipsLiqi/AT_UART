#ifndef __COND_H__
#define __COND_H__

#include "FreeRTOS.h"
#include "semphr.h"


typedef struct {
    xSemaphoreHandle sem;
    xSemaphoreHandle mutex;
}cond_t;

#endif