/*
* @Author: justfortest
* @Date:   2018-01-11 10:34:13
* @Last Modified by:   zxt
* @Last Modified time: 2020-06-08 16:52:11
*/
#include "../general.h"


Semaphore_Struct queueSemStruct;
Semaphore_Handle queueSemHandle;


/***** Function definitions *****/
//***********************************************************************************
// brief:   Init the extflash ring queue parameter
// 
// parameter: 
//***********************************************************************************
void ExtflashRingQueueInit(extflash_queue_s * p_queue)
{  
    static uint8_t init = 0;
    if(init == 0)
    {
        init = 1;
        Semaphore_Params semParams;
        Semaphore_Params_init(&semParams);
        Semaphore_construct(&queueSemStruct, 1, &semParams);
        queueSemHandle = Semaphore_handle(&queueSemStruct);
    }

    Semaphore_pend(queueSemHandle, BIOS_WAIT_FOREVER);
    p_queue->size = EXTFLASH_QUEUE_MAX ;  
     
    p_queue->head = 0;  
    p_queue->tail = 0;  
     
    p_queue->tag  = 0;  
    Semaphore_post(queueSemHandle);
}  




//***********************************************************************************
// brief: add the data to queue   
// 
// parameter: 
// return:
//      true:   add success
//      false:  add fail
//***********************************************************************************
bool ExtflashRingQueuePush(extflash_queue_s * p_queue, uint8_t *data)
{  
    
    Semaphore_pend(queueSemHandle, BIOS_WAIT_FOREVER);

    if(ExtflashRingQueueIsFull(p_queue))
    {  
        Semaphore_post(queueSemHandle);
        return false;  
    }  
        
    memcpy(p_queue->space[p_queue->tail], data, SENSOR_DATA_LENGTH_MAX);
     
    p_queue->tail = (p_queue->tail + 1) % p_queue->size ;  
     
    /* the queue is full*/  
    if(p_queue->tail == p_queue->head)
    {  
       p_queue->tag = 1;  
    }
    Semaphore_post(queueSemHandle);

    return true;
}  
  
//***********************************************************************************
// brief: take the data from queue   
// 
// parameter: 
// return:
//      true:   take success
//      false:  take fail
//***********************************************************************************
bool ExtflashRingQueuePoll(extflash_queue_s * p_queue, uint8_t * data)
{  

    Semaphore_pend(queueSemHandle, BIOS_WAIT_FOREVER);
    if(ExtflashRingQueueIsEmpty(p_queue))  
    {  
        Semaphore_post(queueSemHandle);
        return false;
    }  
    

    memcpy(data, p_queue->space[p_queue->head], SENSOR_DATA_LENGTH_MAX);

    p_queue->head = (p_queue->head + 1) % p_queue->size ;  
     
    /* the queue is empty*/  
    if(p_queue->tail == p_queue->head)  
    {  
        p_queue->tag = 0;  
    }
    
    Semaphore_post(queueSemHandle);

    return true;
}  



