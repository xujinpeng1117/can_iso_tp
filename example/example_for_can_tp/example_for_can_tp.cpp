
#include <iostream>
#include <fstream>
using namespace std;

ofstream output_file;

#include "can_iso_tp_private.h"

static uint8_t dlc2len(uint8_t dlc)
{
	static const uint8_t dlc_len_table[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64 };
	return dlc_len_table[dlc & 0xf];
}
#define MAX_BUFF_LEN 70000
struct can_iso_tp_link_t link[2];
struct can_iso_tp_init_t init[2];
uint8_t rx_buff[2][MAX_BUFF_LEN];
uint8_t payload[2][MAX_BUFF_LEN];

static void print_debug_info(const char* msg)
{
	//e.g. printf this infor to uart port
}
static int L_Data_request(can_iso_tp_link_t_p link_src, const struct CAN_msg* msg)
{
	struct CAN_msg msg_1 = *msg;
	iso_can_tp_L_Data_confirm(link_src, msg, 0);

	{
		char str_msg[600];
		sprintf_s(str_msg, "can msg with id 0x%x dlc 0x%d: ", msg->id.id, msg->dlc);
		for (int i = 0; i < dlc2len(msg->dlc); i++)
		{
			sprintf_s(str_msg, "%s0x%02x ", str_msg, msg->data[i]);
		}
		sprintf_s(str_msg, "%s\n", str_msg);
		output_file << str_msg;
	}

	for (int i = 0; i < 2; i++)
	{
		if (&link[i] != link_src)
		{
			iso_can_tp_L_Data_indication(&link[i], &msg_1);
		}
	}
	return 0;
}
static void N_USData_indication(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error)
{
}
static void N_USData_confirm(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error)
{
}
void init_for_all_links(void)
{
	for (int i = 0; i < sizeof(link) / sizeof(link[0]); i++)
	{
		memset(&link[i], 0, sizeof(struct can_iso_tp_init_t));
		if (i == 0)
		{
			init[i].rx_id.id = 0x601;
			init[i].rx_id.isExt = 0;
			init[i].tx_id.id = 0x602;
			init[i].tx_id.isExt = 0;
			init[i].funtion_id.id = 0x7ff;
		}
		else if (i == 1)
		{
			init[i].rx_id.id = 0x602;
			init[i].rx_id.isExt = 0;
			init[i].tx_id.id = 0x601;
			init[i].tx_id.isExt = 0;
			init[i].funtion_id.id = 0x7ff;
		}
		else {
			init[i].rx_id.id = i * 30;
			init[i].rx_id.isExt = 0;
			init[i].tx_id.id = i * 30 + 1;
			init[i].tx_id.isExt = 0;
			init[i].funtion_id.id = 0x7ff;
		}
		init[i].N_Ar = 1000;
		init[i].N_As = 1000;
		init[i].N_Bs = 1000;
		init[i].N_Cr = 1000;
		init[i].N_WFTmax = 3 + i;
		init[i].L_Data_request = L_Data_request;
		init[i].N_USData_indication = N_USData_indication;
		init[i].N_USData_confirm = N_USData_confirm;
		init[i].rx_buff = rx_buff[i];
		init[i].rx_buff_len = MAX_BUFF_LEN;
		init[i].TX_DLC = 8;
		init[i].frame_pad = 0x77;
		init[i].FC_BS = 0;
		init[i].STmin = 0;
		init[i].print_debug = print_debug_info;
	}
}
int main()
{
	for (int i = 0; i < MAX_BUFF_LEN; i++)
	{
		payload[0][i] = i;
		payload[1][i] = i;
	}

	char file_name[200];
	int test_len, test_TX_DL, test_BS;


	for (test_TX_DL = 8; test_TX_DL <= 0xf; test_TX_DL += 3)
	{
		int len_inc = 1;
		for (test_len = 1; test_len < MAX_BUFF_LEN; test_len += 0xfff/2 + len_inc)
		{
			if (test_len > 0xfff) len_inc = (MAX_BUFF_LEN - 0xfff) / 2;
			for (test_BS = 0; test_BS <= 255; test_BS += 255 / 2)
			{
				init_for_all_links();
				for (int i = 0; i < sizeof(link) / sizeof(link[0]); i++)
				{
					init[i].TX_DLC = test_TX_DL;
					init[i].FC_BS = test_BS;
					iso_can_tp_create(&link[i], &init[i]);
				}
				sprintf_s(file_name, "log_TX_DLC_0x%x_len_%d_bs_%d.txt", test_TX_DL, test_len, test_BS);
				output_file.open(file_name);
				iso_can_tp_N_USData_request(&link[0], 0, payload[0], (uint32_t)test_len);
				output_file.close();
			}
		}
	}
	system("pause");
	return 0;
}

