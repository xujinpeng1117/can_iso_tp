#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "testPrint.h"

#include "can_iso_tp.h"
#include "can_iso_tp_private.h"

#include "test_mocks.h"

#ifdef SUPPORT_CAN_FD
const int TEST_DLC_MAX = 0xf;
#else
const int TEST_DLC_MAX = 8;
#endif
static uint8_t dlc2len(uint8_t dlc)
{
	static const uint8_t dlc_len_table[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64 };
	return dlc_len_table[dlc & 0xf];
}

TEST_GROUP(multiChannelTest)
{
	struct can_iso_tp_init_t init[TEST_CHANNEL_NUM];
	uint8_t rx_buff[MAX_TEST_BUFF_SIZE];
	uint8_t payload[MAX_TEST_BUFF_SIZE];
	void setup()
	{
		print_enable = false;
		init_all_modules();

		for (int i = 0; i < sizeof(payload); i++)
		{
			payload[i] = (uint8_t)i;
		}
		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			memset(&link[i], 0, sizeof(struct can_iso_tp_init_t));
			if (i == 0)
			{
				init[i].rx_id.id = 0x601;
				init[i].rx_id.isExt = 0;
				init[i].rx_id.isCANFD = 0;
				init[i].rx_id.isRemote = 0;
				init[i].tx_id.id = 0x602;
				init[i].tx_id.isExt = 0;
				init[i].tx_id.isCANFD = 0;
				init[i].tx_id.isRemote = 0;
				init[i].funtion_id.id = 0x7ff;
			}
			else if (i == 1)
			{
				init[i].rx_id.id = 0x602;
				init[i].rx_id.isExt = 0;
				init[i].rx_id.isCANFD = 0;
				init[i].rx_id.isRemote = 0;
				init[i].tx_id.id = 0x601;
				init[i].tx_id.isExt = 0;
				init[i].tx_id.isCANFD = 0;
				init[i].tx_id.isRemote = 0;
				init[i].funtion_id.id = 0x7ff;
			}
			else {
				init[i].rx_id.id = i * 30;
				init[i].rx_id.isExt = 0;
				init[i].rx_id.isCANFD = 0;
				init[i].rx_id.isRemote = 0;
				init[i].tx_id.id = i * 30 + 1;
				init[i].tx_id.isExt = 0;
				init[i].tx_id.isCANFD = 0;
				init[i].tx_id.isRemote = 0;
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
			init[i].rx_buff = rx_buff;
			init[i].rx_buff_len = sizeof(rx_buff);
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));
		}
	}
	void teardown()
	{
		destory_all_modules();
	}
};

/*验证多个模块可以配合工作*/
//测试条件：
//1. 测试过程中在L_Data_request函数内调用iso_can_tp_L_Data_confirm,并将多个通道的收和发连在一起
//测试步骤：
//1. 两个通道分别，正常发送多帧，长度为1-4095字节数据
//期待结果：
//双方都能正确接收数据
#if TEST_CHANNEL_NUM>1
int print_msg = 0;
int nomralTxRx_tx_CAN_frame_with_done(can_iso_tp_link_t_p link_tmp, const struct CAN_msg* msg)
{
	struct CAN_msg msg_1 = *msg;
	iso_can_tp_L_Data_confirm(link_tmp, msg, 0);
	if (print_msg != 0)
	{
		printf("can msg: ");
		for (int i = 0; i < 8; i++)
		{
			printf("0x%02x ", msg->data[i]);
		}
		printf("\n");
	}
	for (int i = 0; i < TEST_CHANNEL_NUM; i++)
	{
		if (&link[i] != link_tmp)
		{
			iso_can_tp_L_Data_indication(&link[i], &msg_1);
		}
	}
	return 0;
}

TEST(multiChannelTest, nomralTxRx)
{
	ULONGLONG tick_start = GetTickCount64();
	//print_enable = true;
	for (int TX_DLC = 8; TX_DLC <= TEST_DLC_MAX; TX_DLC++)
	{
		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			testPrintf("test for TEST_DLC=%d  channel=%d", TX_DLC, i);
			init[i].TX_DLC = TX_DLC;
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));
			uint8_t len_start;
			if (TX_DLC == 8)
			{
				len_start = dlc2len(TX_DLC) - 1 + 1;
			}
			else {
				len_start = dlc2len(TX_DLC) - 2 + 1;
			}
			uint64_t len_inc = 1;
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff / 1000 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 15;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------
				L_Data_request_hook = nomralTxRx_tx_CAN_frame_with_done;
				LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[0], 0, payload, (uint32_t)len));
				LONGS_EQUAL(1, last_rx_par[1].cnt);
				LONGS_EQUAL(len, last_rx_par[1].par[0].size);
				LONGS_EQUAL(0, memcmp(last_rx_par[1].par[0].payload, &payload[0], (size_t)len));

				LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[1], 0, payload, (uint32_t)len));
				LONGS_EQUAL(1, last_rx_par[0].cnt);
				LONGS_EQUAL(len, last_rx_par[0].par[0].size);
				LONGS_EQUAL(0, memcmp(last_rx_par[0].par[0].payload, &payload[0], (size_t)len));
				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}
#endif
