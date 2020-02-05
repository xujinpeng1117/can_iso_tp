#pragma once


#include "can_iso_tp.h"
#include "can_iso_tp_private.h"

#include <windows.h>

#define TEST_CHANNEL_NUM 3
#define MAX_TEST_BUFF_SIZE   (165599)

typedef struct {
	struct {
		struct CAN_msg msg;
	}
#if MAX_TEST_BUFF_SIZE / 6 < 100
	par[100];
#else
	par[MAX_TEST_BUFF_SIZE / 6];
#endif
	int cnt;
}last_tx_par_t;

typedef struct {
	struct {
#ifdef SUPPORT_CAN_FD
	#if  MAX_TEST_BUFF_SIZE< 64
			uint8_t payload[64];
	#else
			uint8_t payload[MAX_TEST_BUFF_SIZE];
	#endif
#else
	#if  MAX_TEST_BUFF_SIZE< 8
			uint8_t payload[8];
	#else
			uint8_t payload[MAX_TEST_BUFF_SIZE];
	#endif
#endif
		uint32_t size;
		CAN_ISO_TP_RESAULT error;
	}par[10];
	int cnt;
}last_rx_par_t;

typedef struct {
	struct {
		const uint8_t* payload;
		uint32_t size;
		CAN_ISO_TP_RESAULT error;
	}
#if MAX_TEST_BUFF_SIZE / 6 < 100
	par[100];
#else
	par[MAX_TEST_BUFF_SIZE / 6];
#endif
	int cnt;
}last_tx_done_par_t;

extern struct can_iso_tp_link_t link[TEST_CHANNEL_NUM];
extern void init_all_modules(void);
extern void destory_all_modules(void);
//**************保存上一次请求的发送的报文
extern last_tx_par_t last_tx_par[TEST_CHANNEL_NUM];
extern void init_last_tx_par_vars(void);
extern int (*L_Data_request_hook)(can_iso_tp_link_t_p link, const struct CAN_msg* msg);
extern int L_Data_request(can_iso_tp_link_t_p link, const struct CAN_msg* msg);
//**************保存上一次收到的诊断请求
extern last_tx_done_par_t last_tx_done_par[TEST_CHANNEL_NUM];
extern void (*N_USData_indication_hook)(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error);
extern void init_last_tp_recieve_vars(void);
extern void N_USData_indication(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error);
//**************保存上一次收到的诊断发送完成请求
extern last_rx_par_t last_rx_par[TEST_CHANNEL_NUM];
extern void init_last_tx_done_vars(void);
extern void (*N_USData_confirm_hook)(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error);
extern void N_USData_confirm(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error);

