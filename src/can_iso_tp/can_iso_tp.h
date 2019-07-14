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

#define OP_OK 0
#define OP_NOK 1
typedef struct can_iso_tp_link_t* can_iso_tp_link_t_p;
struct CAN_msg_id {
	uint32_t id:29;   //frame id
	uint32_t isExt:1;//0:std frame， 1：ext frame
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
	int (*L_Data_request)(can_iso_tp_link_t_p link, const struct CAN_msg* msg); //用于调用底层驱动发送CAN报文
	void (*N_USData_indication)(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error);//用于向上层代码汇报收到TP数据
	void (*N_USData_confirm)(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error);//用于向上层代码汇报TP数据发送完成结果
	uint8_t *rx_buff;  //用于多帧接收
	uint32_t rx_buff_len;  //用于多帧接收
	//超时参数
	uint16_t N_As;//发送端将数据传送到接收端的最大时间,默认1000
	uint16_t N_Ar;//接收端将流控制传送到发送端的最大时间,默认1000
	uint16_t N_Bs;//发送端在成功发送首帧后到接收到流控帧的最大时间,默认1000
	//uint16_t N_Br;//接收端在接收到首帧后到发送流控制的最大时间
	//uint16_t N_Cs;//接收端在发送流控制到接收端的最大时间
	uint16_t N_Cr;//接收端在发送成功流控制后到收到连续帧的最大时间,默认1000
	uint8_t N_WFTmax;//发送过程中最长等待次数
	uint8_t FC_BS;//接收过程中发送流控的BS参数
	uint8_t STmin;//接收过程中发送流控的BS参数
	uint8_t TX_DLC;//发送报文的DLC值
	uint8_t frame_pad;
	//可选:打印运行信息
	void(*print_debug)(const char *str);
};


/*
*********************************************************************************************************
*                                        创建传输层通道
* Description : 创建传输层通道
* Arguments   : 传输层通道参数
* Returns    : >=0 TP通道编号，从0开始创建
*              <0 创建失败
*********************************************************************************************************
*/
int iso_can_tp_create(can_iso_tp_link_t_p link, struct can_iso_tp_init_t* init);

/*
*********************************************************************************************************
*                                        循环调用CAN TP模块
* Description : 循环调用CAN TP模块,用于报文处理，超时检查等
* Arguments   : user_ms：当前系统毫秒级时间
* Returns    : 无
*********************************************************************************************************
*/
void iso_can_tp_poll(can_iso_tp_link_t_p link, unsigned int user_ms);

/*
*********************************************************************************************************
*                                        收到CAN报文
* Description : 收到CAN报文
* Arguments   : channel: 通道编号
*               CAN_msg：收到的报文
* Returns    : 0:收到报文与模块对应通道有关
               其他:收到报文与模块对应通道无关
*********************************************************************************************************
*/
int iso_can_tp_L_Data_indication(can_iso_tp_link_t_p link, const struct CAN_msg* msg);
/*
*********************************************************************************************************
*                                        发送CAN报文
* Description : 发送CAN报文
* Arguments   : channel：通道编号
                CAN_msg：发送的报文
* Returns    : 0：成功请求发送
*              其他：请求发送失败
*********************************************************************************************************
*/
//can_iso_tp_link_t结构体中int(*L_Data_request)(can_iso_tp_link_t_p link, const struct CAN_msg* msg);
/*
*********************************************************************************************************
*                                        发送CAN报文完成确认
* Description : 确认发送CAN报文完成
* Arguments   : channel：通道编号
				CAN_msg：发送的报文，返回的是指针，但只保证内容和L_Data_request中对应指针指向内容相同，不用保证是同一指针
				error:
					0: 发送成功
					其他:发送失败
* Returns    : 0：正确确认
*              其他：该报文并非本模块所需报文
*********************************************************************************************************
*/
int iso_can_tp_L_Data_confirm(can_iso_tp_link_t_p link, const struct CAN_msg* msg, int8_t error);

/*
*********************************************************************************************************
*                                        收到传输层数据包
* Description : 收到传输层数据包
* Arguments   : channel：通道编号
                payload：收到的数据
*               size：收到的数据长度
* Returns    : 0：成功请求发送
*              其他：请求发送失败
*********************************************************************************************************
*/
//can_iso_tp_link_t结构体中void (*N_USData_indication)(can_iso_tp_link_t_p link,  const uint8_t payload[], uint32_t size);
/*
*********************************************************************************************************
*                                        发起链路传输请求
* Description : 发起链路传输请求
* Arguments   : channel: 通道编号
*               isFunction：是否为功能寻址，0不是，其他是
				payload：发送数据
				size：发送数据长度
* Returns    : 0：请求成功
*               其他：请求失败
*********************************************************************************************************
*/
int iso_can_tp_N_USData_request(can_iso_tp_link_t_p link, uint8_t isFunction, const uint8_t payload[], uint32_t size);

/*
*********************************************************************************************************
*                                        链路传输请求发送完成确认
* Description : 链路传输请求发送完成确认
* Arguments   : channel: 通道编号
*               isFunction：上一次请求是否为功能寻址
				payload：发送数据指针，指向上一次iso_can_tp_N_USData_request中所请求的数据
				size：上一次请求发送数据长度
				error：发送完成确认结果
					0：正确发送
					其他：发送错误
* Returns    : 
*********************************************************************************************************
*/
//can_iso_tp_link_t结构体中void(*N_USData_confirm)(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, ISOTP_PROTOCOL_RESAULT error);

#ifdef __cplusplus
}
#endif

#endif
