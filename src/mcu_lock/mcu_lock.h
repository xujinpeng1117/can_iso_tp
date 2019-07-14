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
#ifndef _MCU_LOCK_
#define _MCU_LOCK_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

typedef unsigned int mcu_lock_t;
typedef unsigned int cpu_status_t;

#ifdef _WIN32

//In PC test environment, no interruption is needed.
#define MCU_LOCK_ENTER_CRITICAL do{cpu_sr = 1;}while(0)
#define MCU_LOCK_EXIT_CRITICAL do{cpu_sr = 0;}while(0)

#else

//These two macros are related to the specific MCU and need to be modified according to the actual situation.
/* Example£º
#define MCU_LOCK_ENTER_CRITICAL do{\
    cpu_sr=INTERRUPT_STATUS; \
DISABLE_INTERRUPT;}while(0)
#define MCU_LOCK_EXIT_CRITICAL do{INTERRUPT_STATUS = cpu_sr;}while(0)
*/
//Save the current interrupt status and close the interrupt
#define MCU_LOCK_ENTER_CRITICAL do{cpu_sr = 1;}while(0)
//Restore interruption status
#define MCU_LOCK_EXIT_CRITICAL do{cpu_sr = 0;}while(0)

#endif


void mcu_lock_init(mcu_lock_t *lock);
unsigned int mcu_lock_try_lock(register mcu_lock_t *lock);
void mcu_lock_unlock(mcu_lock_t *lock);

#ifdef __cplusplus
}
#endif
#endif
