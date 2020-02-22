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


#include "can_iso_tp.h"
#include "can_iso_tp_private.h"
#include <string.h>


#define INIT_DONE_FLAG ((unsigned int)0x74185295)
#define SET_INIT_DONE_FLAG(link) do{link->init_done_flag = INIT_DONE_FLAG;}while(0)
#define CHECK_INIT_DONE_FLAG(link) (link->init_done_flag == INIT_DONE_FLAG)
#define MODULE_PRINT "can_iso_tp: "
static void printf_debug_msg(struct can_iso_tp_init_t* link, char *msg)
{
	if (link->print_debug)
	{
		link->print_debug(msg);
	}
}
static inline uint8_t dlc2len(uint8_t dlc)
{
	static const uint8_t dlc_len_table[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64 };
	return dlc_len_table[dlc & 0xf];
}
//--------------task mange module----------------
static int event_manage_block_init(struct event_mange_t* event)
{
	mcu_lock_init(&event->lock);
	if (!QueueInit(&event->fifo, event->fifo_data, sizeof(event->fifo_data) / sizeof(event->fifo_data[0])))
	{
		return -1;
	}
	return 0;
}
static int report_event_to_manage_block(struct event_mange_t* task, void* par_with_handle)
{
	int res = OP_NOK;

	//Push events into FIFO queues
	if (QueueOperateOk != QueueIn(&task->fifo, (ElemType)par_with_handle))
	{
		//can not call printf_debug_msg here, return OP_NOK
	}
	else {
		if (mcu_lock_try_lock(&task->lock))
		{
			do {
				struct {
					event_handle_t handle;
				}*par;
				//Retrieve the latest event record from the FIFO queue
				if (QueueOperateOk != QueueOut(&task->fifo, (ElemType*)& par))
				{
					break;
				}
				else {
					if (par->handle)
					{
						par->handle((void*)par);
					}
				}
			} while (1 == 1);
			mcu_lock_unlock(&task->lock);
		}
		res = OP_OK;
	}
	return res;
}
static int lenToMinDlc(uint16_t len)
{
	static const uint8_t dlc_len_table[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64 };
	int dlc;
	for (dlc = 0; dlc <= 15; dlc++)
	{
		if (len <= dlc_len_table[dlc])
			break;
	}
	return dlc;
}
//---------------control logic----------------
static int tx_event_cf_frame(can_iso_tp_link_t_p link)
{
	if (link->tx_record.status == tx_cf_wait_tx)
	{
		int tx_frame = 1;
		if (link->tx_record.rx_Stmin != 0)
		{
			if ((link->current_time_ms - link->tx_record.last_msg_time_ms) < link->tx_record.rx_Stmin)
			{
				tx_frame = 0;
			}
		}
		if (0 != tx_frame)
		{
			uint32_t txLen = link->tx_record.current_size - link->tx_record.current_tx_index;
			if (txLen > (uint32_t)(dlc2len(link->init_info.TX_DLC)-1))
			{
				txLen = (dlc2len(link->init_info.TX_DLC)-1);
			}
			if (txLen == 0)
			{
				link->tx_record.status = tx_idle;
				if (link->init_info.N_USData_confirm)
				{
					link->init_info.N_USData_confirm(link, link->tx_record.current_playload, link->tx_record.current_size, N_OK);
				}
			}
			else {
				link->tx_record.last_msg.id = link->init_info.tx_id;
				link->tx_record.last_msg.data[0] = 0x20 | (link->tx_record.current_tx_SN & 0xf);
				memcpy(&link->tx_record.last_msg.data[1], &link->tx_record.current_playload[link->tx_record.current_tx_index], txLen);
				if (dlc2len(link->init_info.TX_DLC) > (1 + txLen))
				{
					memset(&link->tx_record.last_msg.data[1 + txLen], link->init_info.frame_pad, dlc2len(link->init_info.TX_DLC) - 1 - txLen);
					link->tx_record.last_msg.dlc = lenToMinDlc(txLen+1);
					if (link->tx_record.last_msg.dlc < 8)
						link->tx_record.last_msg.dlc = 8;
				}
				else {
					link->tx_record.last_msg.dlc = link->init_info.TX_DLC;
				}
				link->tx_record.current_tx_index += txLen;
				link->tx_record.status = tx_cf_wait_tx;
				link->tx_record.last_msg_time_ms = link->current_time_ms;
				if (0 == link->init_info.L_Data_request(link, &link->tx_record.last_msg))
				{
					link->tx_record.rx_BS_tx++;
					link->tx_record.current_tx_SN++;
					link->tx_record.status = tx_cf_wait_confirm;
				}
				else {
					link->tx_record.status = tx_cf_wait_tx_retry;
				}
			}
		}
	}
	return 0;
}
static int tx_event_poll(can_iso_tp_link_t_p link, unsigned int user_ms)
{

	if (link->tx_record.status != tx_idle)
	{
		//Check whether the message is sent out of time
		if ((user_ms - link->tx_record.last_msg_time_ms) > link->init_info.N_As)
		{
			if (link->init_info.N_USData_confirm)
			{
				if (link->tx_record.status == tx_wait_fc)
				{
					link->init_info.N_USData_confirm(link, link->tx_record.current_playload, link->tx_record.current_size, N_TIMEOUT_BS);
				}
				else {
					link->init_info.N_USData_confirm(link, link->tx_record.current_playload, link->tx_record.current_size, N_TIMEOUT_A);
				}
			}
			link->tx_record.status = tx_idle;
		}
		else if (link->tx_record.status == tx_sf_wait_tx)
		{
			//If the last driver failed to send, the request is resubmitted
			if (0 == link->init_info.L_Data_request(link, &link->tx_record.last_msg))
			{
				link->tx_record.status = tx_sf_wait_confirm;
			}
		}
		else if (link->tx_record.status == tx_ff_wait_tx)
		{
			//If the last driver failed to send, the request is resubmitted
			if (0 == link->init_info.L_Data_request(link, &link->tx_record.last_msg))
			{
				link->tx_record.status = tx_ff_wait_confirm;
			}
		}
		else if (link->tx_record.status == tx_cf_wait_tx_retry)
		{
			//If the last driver failed to send, the request is resubmitted
			if (0 == link->init_info.L_Data_request(link, &link->tx_record.last_msg))
			{
				link->tx_record.rx_BS_tx++;
				link->tx_record.current_tx_SN++;
				link->tx_record.status = tx_cf_wait_confirm;
			}
		}
		else {
			//do nothing
		}
	}
	tx_event_cf_frame(link);
	return 0;
}
static int tx_event_L_Data_Confirm(can_iso_tp_link_t_p link, int8_t error)
{
	if (link->tx_record.status == tx_sf_wait_confirm)
	{
		if (link->init_info.N_USData_confirm)
		{
			if (0 == error)
			{
				link->init_info.N_USData_confirm(link, link->tx_record.current_playload, link->tx_record.current_size, N_OK);
			}
			else {
				link->init_info.N_USData_confirm(link, link->tx_record.current_playload, link->tx_record.current_size, N_ERROR);
			}
		}
		link->tx_record.status = tx_idle;
	}
	else if (link->tx_record.status == tx_ff_wait_confirm)
	{
		if (link->tx_record.current_size > 0xfff)
		{
			link->tx_record.current_tx_index = dlc2len(link->init_info.TX_DLC) - 6;
		}
		else {
			link->tx_record.current_tx_index = dlc2len(link->init_info.TX_DLC) - 2;
		}
		link->tx_record.current_tx_SN = 1;
		link->tx_record.status = tx_wait_fc;
	}
	else if (link->tx_record.status == tx_cf_wait_confirm)
	{
		if (link->tx_record.rx_BS != 0)
		{
			if ((link->tx_record.rx_BS == link->tx_record.rx_BS_tx)
				&& (link->tx_record.current_size != link->tx_record.current_tx_index)
				)
			{
				link->tx_record.status = tx_wait_fc;
			}
		}
		if (link->tx_record.status == tx_cf_wait_confirm)
		{
			link->tx_record.status = tx_cf_wait_tx;
		}
		tx_event_cf_frame(link);
	}
	return 0;
}
static int tx_event_N_USData_Request(can_iso_tp_link_t_p link, uint8_t isFunction, const uint8_t payload[], uint32_t size)
{
	unsigned int i;
	uint8_t single_frame_max_len = 7;
	if (link->init_info.TX_DLC > 8)
	{
		single_frame_max_len = dlc2len(link->init_info.TX_DLC) - 2;
	}

	if (link->tx_record.status == tx_idle)
	{
		if (size <= single_frame_max_len)
		{
			//single frame
			if (isFunction == 0)
			{
				link->tx_record.last_msg.id = link->init_info.tx_id;
			}
			else {
				link->tx_record.last_msg.id = link->init_info.funtion_id;
			}

			if (size <= 7)
			{
				link->tx_record.last_msg.data[0] = (uint8_t)size;
				for (i = 0; i < size; i++)
				{
					link->tx_record.last_msg.data[1 + i] = payload[i];
				}
				for (; i < single_frame_max_len; i++)
				{
					link->tx_record.last_msg.data[1 + i] = link->init_info.frame_pad;
				}
				link->tx_record.last_msg.dlc = 8;
			}
			else {
				link->tx_record.last_msg.data[0] = 0;
				link->tx_record.last_msg.data[1] = (uint8_t)size;
				for (i = 0; i < size; i++)
				{
					link->tx_record.last_msg.data[2 + i] = payload[i];
				}
				for (; i < single_frame_max_len; i++)
				{
					link->tx_record.last_msg.data[2 + i] = link->init_info.frame_pad;
				}
				link->tx_record.last_msg.dlc = lenToMinDlc(size+2);
			}
			link->tx_record.status = tx_sf_wait_tx;
			link->tx_record.last_msg_time_ms = link->current_time_ms;
			link->tx_record.current_playload = payload;
			link->tx_record.current_size = size;
			if (0 == link->init_info.L_Data_request(link, &link->tx_record.last_msg))
			{
				link->tx_record.status = tx_sf_wait_confirm;
			}
		}
		else {
			//multi frame
			link->tx_record.N_WFT_cnt = 0;
			link->tx_record.last_msg.id = link->init_info.tx_id;
			if (size <= 0xfff)
			{
				link->tx_record.last_msg.data[0] = 0x10 | ((uint8_t)(size >> 8));
				link->tx_record.last_msg.data[1] = (uint8_t)(size & 0xff);
				memcpy(&link->tx_record.last_msg.data[2], payload, dlc2len(link->init_info.TX_DLC) - 2);
			}
			else {
				link->tx_record.last_msg.data[0] = 0x10;
				link->tx_record.last_msg.data[1] = 0;
				link->tx_record.last_msg.data[2] = (uint8_t)(size >> 24);
				link->tx_record.last_msg.data[3] = (uint8_t)(size >> 16);
				link->tx_record.last_msg.data[4] = (uint8_t)(size >> 8);
				link->tx_record.last_msg.data[5] = (uint8_t)(size);
				memcpy(&link->tx_record.last_msg.data[6], payload, dlc2len(link->init_info.TX_DLC) - 6);
			}
			link->tx_record.last_msg.dlc = link->init_info.TX_DLC;
			link->tx_record.status = tx_ff_wait_tx;
			link->tx_record.last_msg_time_ms = link->current_time_ms;
			link->tx_record.current_playload = payload;
			link->tx_record.current_size = size;
			if (0 == link->init_info.L_Data_request(link, &link->tx_record.last_msg))
			{
				link->tx_record.status = tx_ff_wait_confirm;
			}
		}
	}
	else {
		printf_debug_msg(&link->init_info, MODULE_PRINT"can not tx diag request when last request is not done\n");
		if (link->init_info.N_USData_confirm)
		{
			link->init_info.N_USData_confirm(link, payload, size, N_ERROR);
		}
	}
	return 0;
}
static int tx_event_L_Data_indication(can_iso_tp_link_t_p link, const struct CAN_msg* rx_msg)
{
	if (link->tx_record.status == tx_wait_fc)
	{
		if (rx_msg->data[0] != 0x31)
		{
			link->tx_record.N_WFT_cnt = 0;
		}
		if (rx_msg->data[0] == 0x30)
		{
			link->tx_record.status = tx_cf_wait_tx;
			link->tx_record.rx_BS = rx_msg->data[1];
			link->tx_record.rx_BS_tx = 0;
			link->tx_record.rx_Stmin = rx_msg->data[2];
			if (link->tx_record.rx_Stmin > 0x7f)
			{
				link->tx_record.rx_Stmin = 1;
			}
			link->tx_record.last_msg_time_ms = link->current_time_ms - link->tx_record.rx_Stmin - 1;
		}
		else if (rx_msg->data[0] == 0x32)
		{
			link->tx_record.status = tx_idle;
			if (link->init_info.N_USData_confirm)
			{
				link->init_info.N_USData_confirm(link, link->tx_record.current_playload, link->tx_record.current_size, N_BUFFER_OVFLW);
			}
		}
		else if (rx_msg->data[0] == 0x31)
		{
			if (0 != link->init_info.N_WFTmax)
			{
				link->tx_record.N_WFT_cnt++;
				if (link->init_info.N_WFTmax <= link->tx_record.N_WFT_cnt)
				{
					link->tx_record.N_WFT_cnt = 0;
					link->tx_record.status = tx_idle;
				}
			}
		}
		else {
			if (link->init_info.N_USData_confirm)
			{
				link->init_info.N_USData_confirm(link, link->tx_record.current_playload, link->tx_record.current_size, N_INVALID_FS);
			}
			link->tx_record.status = tx_idle;
		}
	}
	tx_event_cf_frame(link);
	return 0;
}
static int rx_event_handle_poll(can_iso_tp_link_t_p link, unsigned int user_ms)
{
	if (link->rx_record.status == rx_wait_cf)
	{
		//Check whether the message is sent out of time
		if ((user_ms - link->rx_record.last_msg_time_ms) > link->init_info.N_Cr)
		{
			if (link->init_info.N_USData_indication)
			{
				link->init_info.N_USData_indication(link, link->init_info.rx_buff, link->rx_record.rx_len, N_TIMEOUT_CR);
			}
			link->rx_record.status = rx_idle;
		}
	}
	if ((link->rx_record.status == rx_tx_fc_wait_confirm) || (link->rx_record.status == rx_tx_fc))
	{
		//Check whether the message is sent out of time
		if ((user_ms - link->rx_record.last_msg_time_ms) > link->init_info.N_Ar)
		{
			if (link->init_info.N_USData_indication)
			{
				link->init_info.N_USData_indication(link, link->init_info.rx_buff, link->rx_record.rx_len, N_TIMEOUT_A);
			}
			link->rx_record.status = rx_idle;
		}
	}
	if (link->rx_record.status == rx_tx_fc)
	{
		//If the last driver failed to send, the request is resubmitted
		if (0 == link->init_info.L_Data_request(link, &link->rx_record.last_msg))
		{
			link->rx_record.status = rx_tx_fc_wait_confirm;
		}
	}
	else if (link->rx_record.status == rx_tx_fc_overrun)
	{
		//If the last driver failed to send, the request is resubmitted
		if (0 == link->init_info.L_Data_request(link, &link->rx_record.last_msg))
		{
			link->rx_record.status = rx_tx_fc_overrun_wait_confirm;
		}
	}
	return 0;
}
static int rx_event_L_Data_Confirm(can_iso_tp_link_t_p link, int8_t error)
{
	if (link->rx_record.status == rx_tx_fc_wait_confirm)
	{
		link->rx_record.status = rx_wait_cf;
	}
	else if (link->rx_record.status == rx_tx_fc_overrun_wait_confirm)
	{
		link->rx_record.status = rx_idle;
	}
	return 0;
}
static int rx_event_L_Data_indication(can_iso_tp_link_t_p link, const struct CAN_msg* rx_msg)
{
	if ((rx_msg->data[0] & 0xf0) == 0)
	{
		if (rx_msg->data[0] != 0)
		{
			if ((rx_msg->dlc <= 8) && (rx_msg->data[0] <= 7))
			{
				uint8_t len = rx_msg->data[0];
				uint8_t rx_len = dlc2len(rx_msg->dlc);
				if (rx_len >= (1 + len))
				{
					if (link->init_info.N_USData_indication)
					{
						if (link->rx_record.status != rx_idle)
						{
							link->init_info.N_USData_indication(link, link->init_info.rx_buff, link->rx_record.rx_len, N_UNEXP_PDU);
						}
						link->init_info.N_USData_indication(link, &rx_msg->data[1], rx_msg->data[0], N_OK);
						link->rx_record.status = rx_idle;
					}
				}
			}
		}
		else {
			if (rx_msg->dlc > 8)
			{
				uint8_t len = rx_msg->data[1];
				if (len > 0)
				{
					uint8_t rx_len = dlc2len(rx_msg->dlc);
					if (rx_len >= (2 + len))
					{
						link->init_info.N_USData_indication(link, &rx_msg->data[2], len, N_OK);
					}
				}
			}
		}
	}
	else if ((rx_msg->data[0] & 0xf0) == 0x10) {
		uint8_t rx_index_offset;
		if (link->rx_record.status != rx_idle)
		{
			link->init_info.N_USData_indication(link, link->init_info.rx_buff, link->rx_record.rx_len, N_UNEXP_PDU);
		}
		//rx ff and send fc
		link->rx_record.rx_len = (rx_msg->data[0] & 0xf) * 256 + rx_msg->data[1];
		if (0 == link->rx_record.rx_len)
		{
			link->rx_record.rx_len = (((uint32_t)rx_msg->data[2]) << 24)
				+ (((uint32_t)rx_msg->data[3]) << 16)
				+ (((uint32_t)rx_msg->data[4]) << 8)
				+ (((uint32_t)rx_msg->data[5]))
				;
			rx_index_offset = 6;
		}
		else {
			rx_index_offset = 2;
		}
		if (link->rx_record.rx_len <= link->init_info.rx_buff_len)
		{
			link->rx_record.rx_index = dlc2len(rx_msg->dlc) - rx_index_offset;
			memcpy(link->init_info.rx_buff, &rx_msg->data[rx_index_offset], link->rx_record.rx_index);
			link->rx_record.rx_SN = 0;
			link->rx_record.tx_BS_cnt = 0;
			link->rx_record.status = rx_tx_fc;
		}
		else {
			link->rx_record.status = rx_tx_fc_overrun;
		}
	}
	else if ((rx_msg->data[0] & 0xf0) == 0x20) {
		//rx cf
		if (link->rx_record.status == rx_wait_cf)
		{
			uint32_t rx_len = link->rx_record.rx_len - link->rx_record.rx_index;
			if (rx_len > (uint32_t)(dlc2len(rx_msg->dlc) - 1))
			{
				rx_len = (dlc2len(rx_msg->dlc) - 1);
			}
			link->rx_record.rx_SN++;
			link->rx_record.last_msg_time_ms = link->current_time_ms;
			if ((link->rx_record.rx_SN & 0xf) == (rx_msg->data[0] & 0xf))
			{
				memcpy(&link->init_info.rx_buff[link->rx_record.rx_index], &rx_msg->data[1], rx_len);
				link->rx_record.rx_index += rx_len;
				if (link->rx_record.status != rx_idle)
				{
					if (link->rx_record.rx_index >= link->rx_record.rx_len)
					{
						if (link->init_info.N_USData_indication)
						{
							link->init_info.N_USData_indication(link, link->init_info.rx_buff, link->rx_record.rx_len, N_OK);
						}
						link->rx_record.status = rx_idle;
					}
					link->rx_record.tx_BS_cnt++;
					if (link->init_info.FC_BS != 0)
					{
						if ((link->rx_record.tx_BS_cnt % link->init_info.FC_BS) == 0)
						{
							link->rx_record.status = rx_tx_fc;
							link->rx_record.tx_BS_cnt = 0;
						}
					}
				}
			}
			else {
				link->init_info.N_USData_indication(link, link->init_info.rx_buff, link->rx_record.rx_len, N_WRONG_SN);
				link->rx_record.status = rx_idle;
			}
		}
	}
	if (link->rx_record.status == rx_tx_fc)
	{
		link->rx_record.last_msg.id = link->init_info.tx_id;
		link->rx_record.last_msg.data[0] = 0x30;
		link->rx_record.last_msg.data[1] = link->init_info.FC_BS;
		link->rx_record.last_msg.data[2] = link->init_info.STmin;
		memset(&link->rx_record.last_msg.data[3], link->init_info.frame_pad, dlc2len(link->init_info.TX_DLC) - 3);
		link->rx_record.last_msg.dlc = link->init_info.TX_DLC;
		link->rx_record.last_msg_time_ms = link->current_time_ms;
		if (0 == link->init_info.L_Data_request(link, &link->rx_record.last_msg))
		{
			link->rx_record.status = rx_tx_fc_wait_confirm;
		}
	}
	else if (link->rx_record.status == rx_tx_fc_overrun)
	{
		link->rx_record.last_msg.id = link->init_info.tx_id;
		link->rx_record.last_msg.data[0] = 0x32;
		link->rx_record.last_msg.data[1] = 0;
		link->rx_record.last_msg.data[2] = 0;
		memset(&link->rx_record.last_msg.data[3], link->init_info.frame_pad, dlc2len(link->init_info.TX_DLC) - 3);
		link->rx_record.last_msg.dlc = link->init_info.TX_DLC;
		link->rx_record.last_msg_time_ms = link->current_time_ms;
		if (0 == link->init_info.L_Data_request(link, &link->rx_record.last_msg))
		{
			link->rx_record.status = rx_tx_fc_overrun_wait_confirm;
		}
	}
	return 0;
}

static void tx_event_poll_handle(void* par_src)
{
	struct time_poll_par_t* par = (struct time_poll_par_t*)par_src;
	tx_event_poll(par->link, par->user_ms);
	par->handle = (event_handle_t)0;
}
static void tx_event_L_Data_Confirm_handle(void* par_src)
{
	struct L_Data_confirm_par_t* par = (struct L_Data_confirm_par_t*)par_src;
	int8_t error = par->error;
	par->handle = (event_handle_t)0;
	tx_event_L_Data_Confirm(par->link, par->error);
}
static void tx_event_N_USData_Request_handle(void* par_src)
{
	struct N_USData_request_par_t* par = (struct N_USData_request_par_t*)par_src;
	tx_event_N_USData_Request(par->link, par->isFunction, par->payload, par->size);
	par->handle = (event_handle_t)0;
}
static void tx_event_L_Data_indication_handle(void* par_src)
{
	struct L_Data_indication_par_t* par = (struct L_Data_indication_par_t*)par_src;
	tx_event_L_Data_indication(par->link, &par->rx_msg);
	par->handle = (event_handle_t)0;
}
static void rx_event_poll_handle(void* par_src)
{
	struct time_poll_par_t* par = (struct time_poll_par_t*)par_src;
	rx_event_handle_poll(par->link, par->user_ms);
	par->handle = (event_handle_t)0;
}
static void rx_event_L_Data_Confirm_handle(void* par_src)
{
	struct L_Data_confirm_par_t* par = (struct L_Data_confirm_par_t*)par_src;
	int8_t error = par->error;
	par->handle = (event_handle_t)0;
	rx_event_L_Data_Confirm(par->link, error);
}
static void rx_event_L_Data_indication_handle(void* par_src)
{
	struct L_Data_indication_par_t* par = (struct L_Data_indication_par_t*)par_src;
	rx_event_L_Data_indication(par->link, &par->rx_msg);
	par->handle = (event_handle_t)0;
}

//---------------------call interface------------------------
int iso_can_tp_create(can_iso_tp_link_t_p link, struct can_iso_tp_init_t* init)
{
	if ((struct can_iso_tp_init_t*)0 == init)
	{
		return OP_NOK;
	}
	if ((can_iso_tp_link_t_p)0 == link)
	{
		return OP_NOK;
	}
	if ((uint8_t*)0 == init->rx_buff)
	{
		printf_debug_msg(init, MODULE_PRINT "null rx_buff\n");
		return OP_NOK;
	}
	if (init->rx_buff_len < 8)
	{
		printf_debug_msg(init, MODULE_PRINT "rx_buff_len should not less than 8\n");
		return OP_NOK;
	}
	if (memcmp(&init->rx_id, &init->tx_id, sizeof(init->tx_id)) == 0)
	{
		printf_debug_msg(init, MODULE_PRINT "rx_id should not = tx_id\n");
		return OP_NOK;
	}
	if (memcmp(&init->funtion_id, &init->tx_id, sizeof(init->tx_id)) == 0)
	{
		printf_debug_msg(init, MODULE_PRINT"funtion_id should not = tx_id\n");
		return OP_NOK;
	}
	if (memcmp(&init->funtion_id, &init->rx_id, sizeof(init->tx_id)) == 0)
	{
		printf_debug_msg(init, MODULE_PRINT"funtion_id should not = rx_id\n");
		return OP_NOK;
	}
	if (init->tx_id.isRemote != 0)
	{
		printf_debug_msg(init, MODULE_PRINT"tx frame should not be remote frame\n");
		init->tx_id.isRemote = 0;
	}
	if (init->funtion_id.isRemote != 0)
	{
		printf_debug_msg(init, MODULE_PRINT"funtion frame should not be remote frame\n");
		init->funtion_id.isRemote = 0;
	}
	if (init->rx_id.isRemote != 0)
	{
		printf_debug_msg(init, MODULE_PRINT"rx frame should not be remote frame\n");
		init->rx_id.isRemote = 0;
	}
	#ifdef SUPPORT_CAN_FD
		if (init->TX_DLC < 8)
		{
			printf_debug_msg(init, MODULE_PRINT"TX_DLC can not less than 8\n");
			init->TX_DLC = 8;
		}
		if (init->TX_DLC > 0xf)
		{
			printf_debug_msg(init, MODULE_PRINT"TX_DLC can not more than 0xf\n");
			init->TX_DLC = 0xf;
		}
	#else
		init->TX_DLC = 8;

		if (init->tx_id.isCANFD != 0)
		{
			printf_debug_msg(init, MODULE_PRINT"tx frame can not have can-fd frame, compile with maro SUPPORT_CAN_FD if you need can-fd support\n");
			return OP_NOK;
		}
		if (init->funtion_id.isCANFD != 0)
		{
			printf_debug_msg(init, MODULE_PRINT"funtion frame can not have can-fd frame, compile with maro SUPPORT_CAN_FD if you need can-fd support\n");
			return OP_NOK;
		}
		if (init->rx_id.isCANFD != 0)
		{
			printf_debug_msg(init, MODULE_PRINT"rx frame can not have can-fd frame, compile with maro SUPPORT_CAN_FD if you need can-fd support\n");
			return OP_NOK;
		}
	#endif
	memset(link, 0, sizeof(struct can_iso_tp_link_t));
	link->init_info = *init;
	if (0 != event_manage_block_init(&link->rx_events.event_manage))
	{
		printf_debug_msg(init, MODULE_PRINT"can not create rx_events mamage block\n");
		return OP_NOK;
	}
	if (0 != event_manage_block_init(&link->tx_events.event_manage))
	{
		printf_debug_msg(init, MODULE_PRINT"can not create tx_events mamage block\n");
		return OP_NOK;
	}
	SET_INIT_DONE_FLAG(link);
	return OP_OK;
}
void iso_can_tp_poll(can_iso_tp_link_t_p link, unsigned int user_ms)
{
	register volatile cpu_status_t cpu_sr;
	if (!CHECK_INIT_DONE_FLAG(link))
	{
		return;
	}
	//Update internal timestamp
	link->current_time_ms = user_ms;
	MCU_LOCK_ENTER_CRITICAL;
	if (link->rx_events.time_poll_par.handle == (event_handle_t)0)
	{
		link->rx_events.time_poll_par.handle = rx_event_poll_handle;
		MCU_LOCK_EXIT_CRITICAL;
		link->rx_events.time_poll_par.link = link;
		link->rx_events.time_poll_par.user_ms = user_ms;
		report_event_to_manage_block(&link->rx_events.event_manage, &link->rx_events.time_poll_par);
	}
	else {
		MCU_LOCK_EXIT_CRITICAL;
		printf_debug_msg(&link->init_info, "iso_can_tp_poll cannot insert new rx evnent when last is not done.\n");
	}

	MCU_LOCK_ENTER_CRITICAL;
	if (link->tx_events.time_poll_par.handle == (event_handle_t)0)
	{
		link->tx_events.time_poll_par.handle = tx_event_poll_handle;
		MCU_LOCK_EXIT_CRITICAL;
		link->tx_events.time_poll_par.link = link;
		link->tx_events.time_poll_par.user_ms = user_ms;
		report_event_to_manage_block(&link->tx_events.event_manage, &link->tx_events.time_poll_par);
	}
	else {
		MCU_LOCK_EXIT_CRITICAL;
		printf_debug_msg(&link->init_info, "iso_can_tp_poll cannot insert new tx evnent when last is not done.\n");
	}

}
int iso_can_tp_L_Data_confirm(can_iso_tp_link_t_p link, const struct CAN_msg* msg, int8_t error)
{
	int res = OP_NOK;
	if((can_iso_tp_link_t_p)0 == link)
	{
		return OP_NOK;
	}
	if (!CHECK_INIT_DONE_FLAG(link))
	{
		return OP_NOK;
	}
	if (msg != (const struct CAN_msg*)0)
	{
		register volatile cpu_status_t cpu_sr;
		//RX task only focuses on sending completed flow control messages, other messages regardless, TX task does not care about sending completed flow control messages
		if ((msg->data[0] & 0xf0) == 0x30)
		{
			if ((msg->id.isExt == link->rx_record.last_msg.id.isExt)
				&& (msg->id.id == link->rx_record.last_msg.id.id)
				&& (msg->dlc == link->rx_record.last_msg.dlc)
				&& (0 == memcmp(msg->data, link->rx_record.last_msg.data, dlc2len(link->rx_record.last_msg.dlc)))
				)
			{
				MCU_LOCK_ENTER_CRITICAL;
				if (link->rx_events.L_Data_confirm_par.handle == (event_handle_t)0)
				{
					link->rx_events.L_Data_confirm_par.handle = rx_event_L_Data_Confirm_handle;
					MCU_LOCK_EXIT_CRITICAL;
					link->rx_events.L_Data_confirm_par.link = link;
					link->rx_events.L_Data_confirm_par.error = error;
					res = report_event_to_manage_block(&link->rx_events.event_manage, &link->rx_events.L_Data_confirm_par);
				}
				else {
					MCU_LOCK_EXIT_CRITICAL;
					printf_debug_msg(&link->init_info, "L_Data_confirm cannot insert new rx evnent when last is not done.\n");
				}

			}
		}
		else {
			if ((msg->id.isExt == link->tx_record.last_msg.id.isExt)
				&& (msg->id.id == link->tx_record.last_msg.id.id)
				&& (msg->dlc == link->tx_record.last_msg.dlc)
				&& (0 == memcmp(msg->data, link->tx_record.last_msg.data, dlc2len(link->tx_record.last_msg.dlc)))
				)
			{
				MCU_LOCK_ENTER_CRITICAL;
				if (link->tx_events.L_Data_confirm_par.handle == (event_handle_t)0)
				{
					link->tx_events.L_Data_confirm_par.handle = tx_event_L_Data_Confirm_handle;
					MCU_LOCK_EXIT_CRITICAL;
					link->tx_events.L_Data_confirm_par.link = link;
					link->tx_events.L_Data_confirm_par.error = error;
					res = report_event_to_manage_block(&link->tx_events.event_manage, &link->tx_events.L_Data_confirm_par);
				}
				else {
					MCU_LOCK_EXIT_CRITICAL;
					printf_debug_msg(&link->init_info, "L_Data_confirm cannot insert new tx evnent when last is not done.\n");
				}
			}
		}
	}
	else {
		//no need to print info
	}
	return res;
}
int iso_can_tp_L_Data_indication(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	int res = OP_NOK;
	if((can_iso_tp_link_t_p)0 == link)
	{
		return OP_NOK;
	}
	if (!CHECK_INIT_DONE_FLAG(link))
	{
		return OP_NOK;
	}
	if (msg)
	{
		//ignore remote frames
		if (msg->id.isRemote != 0)
		{
			return OP_NOK;
		}
#ifndef SUPPORT_CAN_FD
		if ((msg->id.isCANFD != 0) || (msg->dlc > 8))
		{
			printf_debug_msg(&link->init_info, "L_Data_indication cannot handle CANFD frame when SUPPORT_CAN_FD is not defined.\n");
			return OP_NOK;
		}
#endif
		if (((msg->id.isExt == link->init_info.rx_id.isExt)&& (msg->id.id == link->init_info.rx_id.id))
			|| ((msg->id.isExt == link->init_info.funtion_id.isExt)&& (msg->id.id == link->init_info.funtion_id.id))
			)
		{
			register volatile cpu_status_t cpu_sr;
			//TX task only pays attention to receiving flow control message, other receive message can be ignored, RX task no matter receiving flow control message
			if ((msg->data[0] & 0xf0) == 0x30)
			{
				MCU_LOCK_ENTER_CRITICAL;
				if (link->tx_events.L_Data_indication_par.handle == (event_handle_t)0)
				{
					link->tx_events.L_Data_indication_par.handle = tx_event_L_Data_indication_handle;
					MCU_LOCK_EXIT_CRITICAL;
					link->tx_events.L_Data_indication_par.link = link;
					link->tx_events.L_Data_indication_par.rx_msg = *msg;
					res = report_event_to_manage_block(&link->tx_events.event_manage, &link->tx_events.L_Data_indication_par);
				}
				else {
					MCU_LOCK_EXIT_CRITICAL;
					printf_debug_msg(&link->init_info, "L_Data_indication cannot insert new tx evnent when last is not done.\n");
				}
			}
			else {
				MCU_LOCK_ENTER_CRITICAL;
				if (link->rx_events.L_Data_indication_par.handle == (event_handle_t)0)
				{
					link->rx_events.L_Data_indication_par.handle = rx_event_L_Data_indication_handle;
					MCU_LOCK_EXIT_CRITICAL;
					link->rx_events.L_Data_indication_par.link = link;
					link->rx_events.L_Data_indication_par.rx_msg = *msg;
					res = report_event_to_manage_block(&link->rx_events.event_manage, &link->rx_events.L_Data_indication_par);
				}
				else {
					MCU_LOCK_EXIT_CRITICAL;
					printf_debug_msg(&link->init_info, "L_Data_indication cannot insert new rx evnent when last is not done.\n");
				}
			}

		}
	}
	return res;
}
int iso_can_tp_N_USData_request(can_iso_tp_link_t_p link, uint8_t isFunction, const uint8_t payload[], uint32_t size)
{
	int res = OP_NOK;
	if((can_iso_tp_link_t_p)0 == link)
	{
		return OP_NOK;
	}
	if (!CHECK_INIT_DONE_FLAG(link))
	{
		return OP_NOK;
	}
	if ((const uint8_t*)0 == payload)
	{
		res = OP_NOK;
		printf_debug_msg(&link->init_info, MODULE_PRINT"can not tx diag request when payload == (const uint8_t*)0\n");
	}
	else {
		if (0 == size)
		{
			res = OP_NOK;
			printf_debug_msg(&link->init_info, MODULE_PRINT"can not tx diag request when size == 0\n");
		}
		else {
			register volatile cpu_status_t cpu_sr;
			MCU_LOCK_ENTER_CRITICAL;
			if (link->tx_events.N_USData_request_par.handle == (event_handle_t)0)
			{
				link->tx_events.N_USData_request_par.handle = tx_event_N_USData_Request_handle;
				MCU_LOCK_EXIT_CRITICAL;
				link->tx_events.N_USData_request_par.link = link;
				link->tx_events.N_USData_request_par.isFunction = isFunction;
				link->tx_events.N_USData_request_par.payload = payload;
				link->tx_events.N_USData_request_par.size = size;
				res = report_event_to_manage_block(&link->tx_events.event_manage, &link->tx_events.N_USData_request_par);
			}
			else {
				MCU_LOCK_EXIT_CRITICAL;
				printf_debug_msg(&link->init_info, "N_USData_request cannot insert new tx evnent when last is not done.\n");
			}
		}
	}
	return res;
}
