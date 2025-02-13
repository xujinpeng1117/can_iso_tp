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
#ifndef CAN_ISO_TP_H
#define CAN_ISO_TP_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
//Support CAN_FD need more ram
#define SUPPORT_CAN_FD
#define CAN_ISO_TP_TX_FC_DLC (8) //The DLC value is not explicitly stated in the specification, so 8 is used here, and developers can modify it as needed

#define OP_OK 0
#define OP_NOK 1
typedef struct can_iso_tp_link_t* can_iso_tp_link_t_p;
struct CAN_msg_id {
	uint32_t id:29;   //frame id
	uint32_t isExt:1;//0:std frame, 1: ext frame
	uint32_t isCANFD : 1;//0:classical frame, 1: CANFD frame
	uint32_t isRemote : 1;//0:data frame, 1: remote frame
};
struct CAN_msg {
	struct CAN_msg_id id;
	uint8_t dlc;
#ifdef SUPPORT_CAN_FD
	uint8_t data[64];
#else
	uint8_t data[8];
#endif
};
typedef enum {
	N_OK
	, N_TIMEOUT_A
	, N_TIMEOUT_BS
	, N_TIMEOUT_CR
	, N_WRONG_SN
	, N_INVALID_FS
	, N_UNEXP_PDU
	, N_WFT_OVRN
	, N_BUFFER_OVFLW
	, N_ERROR
}CAN_ISO_TP_RESAULT;
struct can_iso_tp_init_t {
	void* usr_arg;  //used by user
	struct CAN_msg_id tx_id; //tx id when tx tp frame
	struct CAN_msg_id rx_id; //rx id when rx tp frame
	struct CAN_msg_id funtion_id; //tx id when tx tp function frame
	int (*L_Data_request)(can_iso_tp_link_t_p link, const struct CAN_msg* msg); //Used to call the low level driver to send CAN messages
	void (*N_USData_indication)(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error);//Reporting Received TP Data to Upper layer
	void (*N_USData_confirm)(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error);//Used to report TP data completion results to the upper layer
	uint8_t *rx_buff;  //For multi-frame reception, point to rx buff
	uint32_t rx_buff_len;  //For multi-frame reception, rx buff length
	//Timeout parameter
	uint16_t N_As;//Maximum time for the sender to transmit data to the receiver, default 1000
	uint16_t N_Ar;//Maximum time for the receiver to transmit flow control to the sender, default 1000
	uint16_t N_Bs;//The maximum time that the sender receives a flow controll frame after successfully sending the first frame, 1000 by default.
	//uint16_t N_Br;//Maximum time between receiving end and sending flow control after receiving the first frame
	//uint16_t N_Cs;//Maximum time that the receiving end controls the sending flow to the receiving end
	uint16_t N_Cr;//The maximum time from sending successful flow control to receiving continuous frames, 1000 by default.
	uint8_t N_WFTmax;//Maximum number of waiting times during sending
	uint8_t FC_BS;//BS parameters of flow control in receiving process
	uint8_t STmin;//STmin parameters of flow control in receiving process
	uint8_t TX_DLC;//DLC value of sending message, only used for CAN-FD
	uint8_t frame_pad;
	//Optional: Print run information
	void(*print_debug)(const char *str);
};


/*
*********************************************************************************************************
*                                        Creating Transport Layer Channels
* Description : Creating Transport Layer Channels
* Arguments   : link: CAN link pointer
				init: init parameters
* Returns    : OP_OK: create success
*              OP_NOK: create fail
*********************************************************************************************************
*/
int iso_can_tp_create(can_iso_tp_link_t_p link, struct can_iso_tp_init_t* init);

/*
*********************************************************************************************************
*                                        cycle call CAN TP module
* Description : Loop call CAN TP module for frame processing, timeout checking, etc.
* Arguments   : link: CAN link pointer
                user_ms：Current system time in millisecond
* Returns    : none
*********************************************************************************************************
*/
void iso_can_tp_poll(can_iso_tp_link_t_p link, unsigned int user_ms);

/*
*********************************************************************************************************
*                                        Received CAN frame from lower layer
* Description : Received CAN frame from lower layer
* Arguments   : link: CAN link pointer
*               CAN_msg：recieved CAN frame
* Returns    : 0: Receiving a message is related to the module's corresponding channel
               other: Receiving a message has nothing to do with the corresponding channel of the module
*********************************************************************************************************
*/
int iso_can_tp_L_Data_indication(can_iso_tp_link_t_p link, const struct CAN_msg* msg);
/*
*********************************************************************************************************
*                                        send CAN frame
* Description : send CAN frame, init by user
* Arguments   : link: CAN link pointer
                CAN_msg：CAN frame to be send
* Returns    : 0：Successful request sending
*              other：Failed to send request
*********************************************************************************************************
*/
//can_iso_tp_link_t:   int(*L_Data_request)(can_iso_tp_link_t_p link, const struct CAN_msg* msg);
/*
*********************************************************************************************************
*                                        Confirm to the last CAN frame tx
* Description : Confirm to the last CAN frame tx
* Arguments   : link: CAN link pointer
				CAN_msg：The frame sent by lower layer
				error:
					0: Successful delivery
					other: Fail in send
* Returns    : 0：Correct confirmation
*              other：This frame is not required by this link.
*********************************************************************************************************
*/
int iso_can_tp_L_Data_confirm(can_iso_tp_link_t_p link, const struct CAN_msg* msg, int8_t error);

/*
*********************************************************************************************************
*                                        Receiving Transport Layer Packets
* Description : Receiving Transport Layer Packets
* Arguments   : link: CAN link pointer
                payload：rx data pointer
*               size：rx data size
* Returns    : none
*********************************************************************************************************
*/
//can_iso_tp_link_t结构体中void (*N_USData_indication)(can_iso_tp_link_t_p link,  const uint8_t payload[], uint32_t size);
/*
*********************************************************************************************************
*                                        Initiate link transfer request
* Description : Initiate link transfer request
* Arguments   : link: CAN link pointer
*               isFunction：Whether addressing for function, 0 is not
				payload：tx data pointer
				size：tx data size
* Returns    : 0：Successful request
*               other：request fail
*********************************************************************************************************
*/
int iso_can_tp_N_USData_request(can_iso_tp_link_t_p link, uint8_t isFunction, const uint8_t payload[], uint32_t size);

/*
*********************************************************************************************************
*                                        Link Transport Request Sending Complete Confirmation
* Description : Link Transport Request Sending Complete Confirmation, init by user
* Arguments   : link: CAN link pointer
*               isFunction：Whether addressing for function, 0 is not
				payload：data pointer requested in the previous iso_can_tp_N_USData_request
				size：Last request sent data length
				error：Send Completion Confirmation Result
* Returns    : 
*********************************************************************************************************
*/
//can_iso_tp_link_t结构体中void(*N_USData_confirm)(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, ISOTP_PROTOCOL_RESAULT error);

#ifdef __cplusplus
}
#endif

#endif
