#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "testPrint.h"
#include "test_mocks.h"
#include <vector>

#define ARRAY_OF(x) (sizeof(x)/sizeof(x[0]))

struct can_iso_tp_link_t link[TEST_CHANNEL_NUM];

last_tx_par_t last_tx_par[TEST_CHANNEL_NUM];
int (*L_Data_request_hook)(can_iso_tp_link_t_p link, const struct CAN_msg* msg);

last_tx_done_par_t last_tx_done_par[TEST_CHANNEL_NUM];
void (*N_USData_indication_hook)(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error);

last_rx_par_t last_rx_par[TEST_CHANNEL_NUM];
void (*N_USData_confirm_hook)(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error);

static inline int can_iso_tp_link_t_p2index(can_iso_tp_link_t_p link_p)
{
	for (int i = 0; i < TEST_CHANNEL_NUM; i++)
	{
		if (link_p == &link[i])
			return i;
	}
	return -1;
}
void init_all_modules(void)
{
	init_last_tx_par_vars();
	init_last_tp_recieve_vars();
	init_last_tx_done_vars();

	L_Data_request_hook = NULL;
	N_USData_indication_hook = NULL;
	N_USData_confirm_hook = NULL;

	memset(link, 0, sizeof(link));
}
void destory_all_modules(void)
{
	memset(link, 0, sizeof(link));
}

//保存上一次请求的发送的报文
void init_last_tx_par_vars(void)
{
	for (int i = 0; i < TEST_CHANNEL_NUM; i++)
	{
		last_tx_par[i].cnt = 0;
	}
}
int L_Data_request(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	int res = 0;
	int i = can_iso_tp_link_t_p2index(link);
	if (i >= 0)
	{
		if (last_tx_par[i].cnt < ARRAY_OF(last_tx_par[i].par))
		{
			last_tx_par[i].par[last_tx_par[i].cnt].msg = *msg;
			last_tx_par[i].cnt++;
		}
		else {
			printf("no enough mem for L_Data_request\n");
		}
	}
	else {
		printf("unexpect pointer to L_Data_request\n");
	}


	if (L_Data_request_hook)
	{
		res = L_Data_request_hook(link, msg);
	}

	return res;
}

//保存上一次收到的诊断请求
void init_last_tp_recieve_vars(void)
{
	for (int i = 0; i < TEST_CHANNEL_NUM; i++)
	{
		last_rx_par[i].cnt = 0;
	}
}
void N_USData_indication(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error)
{
	int i = can_iso_tp_link_t_p2index(link);
	if (i >= 0)
	{
		if (last_rx_par[i].cnt < ARRAY_OF(last_rx_par[i].par))
		{
			if (size > 0)
			{
				if (size <= sizeof(last_rx_par[i].par[last_rx_par[i].cnt].payload))
					memcpy(last_rx_par[i].par[last_rx_par[i].cnt].payload, payload, size);
				else {
					printf("MAX_TEST_BUFF_SIZE is less than size in N_USData_indication");
					memcpy(last_rx_par[i].par[last_rx_par[i].cnt].payload, payload, MAX_TEST_BUFF_SIZE);
				}
			}
			last_rx_par[i].par[last_rx_par[i].cnt].size = size;
			last_rx_par[i].par[last_rx_par[i].cnt].error = error;
			last_rx_par[i].cnt++;
		}
		else {
			printf("no enough mem for N_USData_indication\n");
		}
	}
	else {
		printf("unexpect pointer to N_USData_indication\n");
	}

	if (N_USData_indication_hook)
	{
		N_USData_indication_hook(link, payload, size, error);
	}
}
//保存上一次收到的诊断发送完成请求
last_tx_done_par_t par_tx_done;
void init_last_tx_done_vars(void)
{
	for (int i = 0; i < TEST_CHANNEL_NUM; i++)
	{
		last_tx_done_par[i].cnt = 0;
	}
}
void N_USData_confirm(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error)
{

	int i = can_iso_tp_link_t_p2index(link);
	if (i >= 0)
	{
		if (last_tx_done_par[i].cnt < ARRAY_OF(last_tx_done_par[i].par))
		{
			last_tx_done_par[i].par[last_tx_done_par[i].cnt].error = error;
			last_tx_done_par[i].par[last_tx_done_par[i].cnt].size = size;
			last_tx_done_par[i].par[last_tx_done_par[i].cnt].payload = payload;
			last_tx_done_par[i].cnt++;
		}
		else {
			printf("no enough mem for N_USData_confirm\n");
		}
	}
	else {
		printf("unexpect pointer to N_USData_confirm\n");
	}

	if (N_USData_confirm_hook)
	{
		N_USData_confirm_hook(link, payload, size, error);
	}
}
