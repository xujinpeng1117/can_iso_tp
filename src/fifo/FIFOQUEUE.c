/*
BSD 2-Clause License

Copyright (c) 2019, xujinpeng1117
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "FIFOQUEUE.h"
#include "mcu_lock.h"
/*Queue Init*/
uint8_t
QueueInit(struct FifoQueue* Queue, ElemType* dat, uint16_t queue_size)
{
	uint8_t ret = (0==1);
	Queue->queue_size = queue_size;
	Queue->dat = dat;
	if ((Queue->dat != (ElemType*)0)
		&& (Queue->queue_size != 0)
		)
	{
		Queue->front = 0;
		Queue->rear = 0;;
		Queue->count = 0;
		ret = (1 == 1);
	}
	return ret;
}

/* Queue In */
uint8_t
QueueIn (register struct FifoQueue *Queue, ElemType sdat)
{
	register volatile cpu_status_t cpu_sr;
	MCU_LOCK_ENTER_CRITICAL;
    if ((Queue->front == Queue->rear) && (Queue->count == Queue->queue_size))
    {
		MCU_LOCK_EXIT_CRITICAL;
        return QueueFull;
    }
    else
    {
        Queue->dat[Queue->rear] = sdat;
        Queue->rear = (Queue->rear + 1) % Queue->queue_size;
        Queue->count = Queue->count + 1;
		MCU_LOCK_EXIT_CRITICAL;
        return QueueOperateOk;
    }
}

/* Queue Out*/
uint8_t
QueueOut (register struct FifoQueue * Queue, ElemType * sdat)
{
	register volatile cpu_status_t cpu_sr;
	MCU_LOCK_ENTER_CRITICAL;
    if ((Queue->front == Queue->rear) && (Queue->count == 0))
    {
		MCU_LOCK_EXIT_CRITICAL;
        return QueueEmpty;
    }
    else
    {
        *sdat = Queue->dat[Queue->front];
        Queue->front = (Queue->front + 1) % Queue->queue_size;
        Queue->count = Queue->count - 1;
		MCU_LOCK_EXIT_CRITICAL;
        return QueueOperateOk;
    }
}
