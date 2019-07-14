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
#ifndef CAN_ISO_TP_PRIVATE_H
#define CAN_ISO_TP_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include "can_iso_tp.h"
#include "mcu_lock.h"
#include "FIFOQUEUE.h"

typedef void(*event_handle_t)(void*);
struct time_poll_par_t {
	event_handle_t handle; //handle == 0 means this parameter is idle
	can_iso_tp_link_t_p link;
	unsigned int user_ms;
};
struct L_Data_confirm_par_t {
	event_handle_t handle; //handle == 0 means this parameter is idle
	can_iso_tp_link_t_p link;
	int8_t error;
};
struct N_USData_request_par_t {
	event_handle_t handle; //handle == 0 means this parameter is idle
	can_iso_tp_link_t_p link;
	uint8_t isFunction;
	uint32_t size;
	const uint8_t* payload;
};
struct L_Data_indication_par_t {
	event_handle_t handle; //handle == 0 means this parameter is idle
	can_iso_tp_link_t_p link;
	struct CAN_msg rx_msg;
};
#define MAX_EVENT (3)
struct event_mange_t {
	mcu_lock_t lock;
	ElemType fifo_data[MAX_EVENT];
	struct FifoQueue fifo;
};

typedef enum {
	tx_idle = 0
	, tx_sf_wait_tx
	, tx_sf_wait_confirm

	, tx_ff_wait_tx
	, tx_ff_wait_confirm

	, tx_wait_fc

	, tx_cf_wait_tx
	, tx_cf_wait_tx_retry
	, tx_cf_wait_confirm
}e_tx_status;
typedef enum {
	rx_idle = 0
	, rx_tx_fc
	, rx_tx_fc_wait_confirm
	, rx_wait_cf

	, rx_tx_fc_overrun
	, rx_tx_fc_overrun_wait_confirm
}e_rx_status;


struct can_iso_tp_link_t {
	struct can_iso_tp_init_t init_info;
	//通用变量
	unsigned int init_done_flag;
	unsigned int current_time_ms; //当前时间戳
	//发送任务相关变量
	struct {
		struct CAN_msg last_msg; //上一次发送的报文，用于判断上一次是否成功发送
		unsigned int last_msg_time_ms; //上一次发送的报文的时间戳，用于判断是否发送超时
		const uint8_t *current_playload; //当前应用层请求发送的数据指针
		uint32_t current_size; //当前应用层请求发送的数据长度
		uint32_t current_tx_index; //当前已经发送的数据长度，只用于多帧传输
		uint8_t current_tx_SN; //当前已经发送的数据计数值，只用于多帧传输
		uint8_t rx_BS;//当前收到的bs参数
		uint8_t rx_BS_tx;//当前bs块已经发送的报文数量
		uint8_t rx_Stmin;//当前收到的Stmin参数
		uint8_t N_WFT_cnt;
		e_tx_status status; //记录当前发送状态
	}tx_record;
	struct {
		struct L_Data_indication_par_t L_Data_indication_par;
		struct L_Data_confirm_par_t L_Data_confirm_par;
		struct time_poll_par_t time_poll_par;
		struct N_USData_request_par_t N_USData_request_par;
		struct event_mange_t event_manage;
	}tx_events;
	//接收任务相关变量
	struct {
		e_rx_status status;
		uint32_t rx_index;  
		uint32_t rx_len; 
		uint8_t rx_SN;
		uint8_t tx_BS_cnt;
		struct CAN_msg last_msg; //上一次发送的报文，用于判断上一次流控是否成功发送
		unsigned int last_msg_time_ms; //上一次发送的报文的时间戳，用于判断流控是否发送超时
	}rx_record;
	struct {
		struct L_Data_indication_par_t L_Data_indication_par;
		struct L_Data_confirm_par_t L_Data_confirm_par;
		struct time_poll_par_t time_poll_par;
		struct event_mange_t event_manage;
	}rx_events;
} ;

#ifdef __cplusplus
}
#endif
#endif