#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "testPrint.h"
#include "test_mocks.h"


#include "can_iso_tp.h"
#include "can_iso_tp_private.h"

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

static void print_debug_info(const char* msg)
{
	testPrintf(msg);
}


TEST_GROUP(singleTest)
{
	struct can_iso_tp_init_t init[TEST_CHANNEL_NUM];
	uint8_t rx_buff[4095];
	uint8_t payload[64];
	uint8_t payload1[64];
	uint8_t payload2[64];
	void setup()
	{
		print_enable = false;
		init_all_modules();

		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			memset(&init[i], 0, sizeof(struct can_iso_tp_init_t));
			init[i].rx_id.id = i*50;
			init[i].rx_id.isExt = 0;
			init[i].rx_id.isCANFD = 0;
			init[i].rx_id.isRemote = 0;
			init[i].tx_id.id = i * 50+1;
			init[i].tx_id.isExt = 0;
			init[i].tx_id.isCANFD = 0;
			init[i].tx_id.isRemote = 0;
			init[i].funtion_id.id = i * 50+2;
			init[i].funtion_id.isExt = 0;
			init[i].funtion_id.isCANFD = 0;
			init[i].funtion_id.isRemote = 0;
			init[i].N_Ar = 1000;
			init[i].N_As = 900;
			init[i].N_Bs = 800;
			init[i].N_Cr = 700;
			init[i].L_Data_request = L_Data_request;
			init[i].N_USData_indication = N_USData_indication;
			init[i].N_USData_confirm = N_USData_confirm;
			init[i].rx_buff = rx_buff;
			init[i].rx_buff_len = sizeof(rx_buff);
			init[i].TX_DLC = 8;
			init[i].print_debug = print_debug_info;
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));
		}

		for (int i = 0; i < sizeof(payload); i++)
		{
			payload[i] = (uint8_t)i;
			payload1[i] = (uint8_t)1 + i;
			payload2[i] = (uint8_t)2 + i;
		}
	}
	void teardown()
	{
		destory_all_modules();
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
	void singleFrameTxTest(int index, uint8_t isFunction, const uint8_t* payload, uint16_t len)
	{
		int i;
		init_last_tx_par_vars();
		init_last_tx_done_vars();
		LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[index], isFunction, payload, len));
		LONGS_EQUAL(1, last_tx_par[index].cnt);
		if (0 == isFunction)
		{
			LONGS_EQUAL(init[index].tx_id.isExt, last_tx_par[index].par[0].msg.id.isExt);
			LONGS_EQUAL(init[index].tx_id.id, last_tx_par[index].par[0].msg.id.id);
			LONGS_EQUAL(init[index].tx_id.isCANFD, last_tx_par[index].par[0].msg.id.isCANFD);
		}
		else {
			LONGS_EQUAL(init[index].funtion_id.isExt, last_tx_par[index].par[0].msg.id.isExt);
			LONGS_EQUAL(init[index].funtion_id.id, last_tx_par[index].par[0].msg.id.id);
			LONGS_EQUAL(init[index].funtion_id.isCANFD, last_tx_par[index].par[0].msg.id.isCANFD);
		}
		LONGS_EQUAL(0, last_tx_par[index].par[0].msg.id.isRemote);
		//according 15765-2016 table12 and table13,bug reported by songarpore@hotmail.com
		if (len <= 7)
		{
			LONGS_EQUAL(8, last_tx_par[index].par[0].msg.dlc);
		}
		else {
			LONGS_EQUAL(lenToMinDlc(len+2), last_tx_par[index].par[0].msg.dlc);
		}

		if (last_tx_par[index].par[0].msg.dlc <= 8)
		{
			LONGS_EQUAL(len, last_tx_par[index].par[0].msg.data[0]);
			for (i = 0; i < len; i++)
			{
				LONGS_EQUAL(payload[i], last_tx_par[index].par[0].msg.data[1 + i]);
			}
			for (; i < (8 - 1); i++)
			{
				LONGS_EQUAL(init[index].frame_pad, last_tx_par[index].par[0].msg.data[1 + i]);
			}
		}
		else {
			LONGS_EQUAL(0, last_tx_par[index].par[0].msg.data[0]);
			LONGS_EQUAL(len, last_tx_par[index].par[0].msg.data[1]);
			for (i = 0; i < len; i++)
			{
				LONGS_EQUAL(payload[i], last_tx_par[index].par[0].msg.data[2 + i]);
			}
			for (; i < (dlc2len(last_tx_par[index].par[0].msg.dlc) - 2); i++)
			{
				LONGS_EQUAL(init[index].frame_pad, last_tx_par[index].par[0].msg.data[2 + i]);
			}
		}

		LONGS_EQUAL(0, iso_can_tp_L_Data_confirm(&link[index], &last_tx_par[index].par[0].msg, 0));
		LONGS_EQUAL(1, last_tx_done_par[index].cnt);
		LONGS_EQUAL(N_OK, last_tx_done_par[index].par[0].error);
		LONGS_EQUAL(len, last_tx_done_par[index].par[0].size);
		CHECK(0 == memcmp(last_tx_done_par[index].par[0].payload, payload, len));
	}
	void singleFrameTxTest(int index, const uint8_t*payload, uint16_t len)
	{
		singleFrameTxTest(index, 0, payload, len);
	}

	void singleFrameRxTest(int index, uint8_t isFunction, const uint8_t* payload, int dlc)
	{
		int test_size;
		if (dlc > 8) test_size = payload[1];
		else test_size = payload[0];
		struct CAN_msg msg;
		msg.dlc = dlc;
		if (0 == isFunction)
		{
			msg.id = init[index].rx_id;
		}
		else {
			msg.id = init[index].funtion_id;
		}
		int i;
		memcpy(msg.data, payload, dlc2len(dlc));

		init_last_tp_recieve_vars();
		iso_can_tp_L_Data_indication(&link[index], &msg);
		LONGS_EQUAL(1, last_rx_par[index].cnt);
		LONGS_EQUAL(test_size, last_rx_par[index].par[0].size);
		if (dlc > 8)
		{
			for (i = 0; i < test_size; i++)
			{
				LONGS_EQUAL(payload[i + 2], last_rx_par[index].par[0].payload[i]);
			}
		}
		else {
			for (i = 0; i < test_size; i++)
			{
				LONGS_EQUAL(payload[i + 1], last_rx_par[index].par[0].payload[i]);
			}
		}

	}
	void singleFrameRxTest(int index, const uint8_t*payload, int dlc)
	{
		singleFrameRxTest(index, 0, payload, dlc);
	}
};
//正确初始化后发送单帧和广播
TEST(singleTest, singleTx)
{
	ULONGLONG tick_start = GetTickCount64();
	//print_enable = true;
	for (int TX_DLC = 8; TX_DLC <= TEST_DLC_MAX; TX_DLC++)
	{
		uint8_t len = dlc2len(TX_DLC) - 2;
		if (TX_DLC == 8) len = 7;
		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			testPrintf("test for TEST_DLC=%d  channel=%d", TX_DLC, i);
			init[i].TX_DLC = TX_DLC;
			if (TX_DLC == 8) {
				init[i].tx_id.isCANFD = 0;
				init[i].funtion_id.isCANFD = 0;
			}
			else {
				init[i].tx_id.isCANFD = 1;
				init[i].funtion_id.isCANFD = 1;
			}
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));

			for (int len_sub = 1; len_sub <= len; len_sub++)
			{
				singleFrameTxTest(i, payload, len_sub);
				singleFrameTxTest(i, 1, payload, len_sub);
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}
//发送单帧后不确认发送完成，再次请求发送单帧时不发送并报错
TEST(singleTest, singleTxWithoutDone)
{
	ULONGLONG tick_start = GetTickCount64();
	//print_enable = true;
	for (int TX_DLC = 8; TX_DLC <= TEST_DLC_MAX; TX_DLC++)
	{
		uint8_t len = dlc2len(TX_DLC) - 2;
		if (TX_DLC == 8) len = 7;
		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			testPrintf("test for TEST_DLC=%d  channel=%d", TX_DLC, i);
			init[i].TX_DLC = TX_DLC;
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));

			LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, len));
			init_last_tx_done_vars();
			LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload1, len));
			CHECK(N_OK != last_tx_done_par[i].par[0].error);
			LONGS_EQUAL(len, last_tx_done_par[i].par[0].size);
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}


//发送单帧后确认发送完成报文并不是请求报文，再次请求发送单帧时不发送并报错
TEST(singleTest, singleTxWithOtherDone)
{
	ULONGLONG tick_start = GetTickCount64();
	//print_enable = true;
	for (int TX_DLC = 8; TX_DLC <= TEST_DLC_MAX; TX_DLC++)
	{
		uint8_t len = dlc2len(TX_DLC) - 2;
		if (TX_DLC == 8) len = 7;
		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			testPrintf("test for TEST_DLC=%d  channel=%d", TX_DLC, i);
			init[i].TX_DLC = TX_DLC;
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));

			init_last_tx_par_vars();
			LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, len));
			LONGS_EQUAL(1, last_tx_par[i].cnt);
			//长度不匹配
			struct CAN_msg wrong_done_msg = last_tx_par[i].par[0].msg;
			wrong_done_msg.dlc--;
			CHECK(0 != iso_can_tp_L_Data_confirm(&link[i], &wrong_done_msg, 0));
			//id类型不匹配
			wrong_done_msg = last_tx_par[i].par[0].msg;
			wrong_done_msg.id.isExt = !wrong_done_msg.id.isExt;
			CHECK(0 != iso_can_tp_L_Data_confirm(&link[i], &wrong_done_msg, 0));
			//id不匹配
			wrong_done_msg = last_tx_par[i].par[0].msg;
			wrong_done_msg.id.id++;
			CHECK(0 != iso_can_tp_L_Data_confirm(&link[i], &wrong_done_msg, 0));
			//数据段不匹配-第一字节
			wrong_done_msg = last_tx_par[i].par[0].msg;
			wrong_done_msg.data[0]++;
			CHECK(0 != iso_can_tp_L_Data_confirm(&link[i], &wrong_done_msg, 0));

			//数据段不匹配-最后字节   - 该bug由songarpore@hotmail.com提供
			wrong_done_msg = last_tx_par[i].par[0].msg;
			wrong_done_msg.data[dlc2len(wrong_done_msg.dlc)-1]++;
			CHECK(0 != iso_can_tp_L_Data_confirm(&link[i], &wrong_done_msg, 0));
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}


//发送单帧后不确认发送完成，再调用poll并且和发送行为发生上一次poll之间时间间隔大于CAN_FRAME_TX_TIMEOUT_MS时，重新允许新的单帧发送，第二次poll时通过N_USData_confirm向上层汇报超时故障N_TIMEOUT_A
TEST(singleTest, singleTxWithDoneTimeout)
{
	ULONGLONG tick_start = GetTickCount64();
	//print_enable = true;
	for (int TX_DLC = 8; TX_DLC <= TEST_DLC_MAX; TX_DLC++)
	{
		uint8_t len = dlc2len(TX_DLC) - 2;
		if (TX_DLC == 8) len = 7;
		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			testPrintf("test for TEST_DLC=%d  channel=%d", TX_DLC, i);
			init[i].TX_DLC = TX_DLC;
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));

			iso_can_tp_poll(&link[i], 0);
			iso_can_tp_poll(&link[i], 4);
			LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, len));

			init_last_tx_done_vars();
			iso_can_tp_poll(&link[i], 4 + init[i].N_As + 1);
			LONGS_EQUAL(1, last_tx_done_par[i].cnt);
			LONGS_EQUAL(len, last_tx_done_par[i].par[0].size);
			POINTERS_EQUAL(payload, last_tx_done_par[i].par[0].payload);
			LONGS_EQUAL(N_TIMEOUT_A, last_tx_done_par[i].par[0].error);

			singleFrameTxTest(i, payload2, len);
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}
//iso_can_tp_L_Data_confirm在发送回调中被调用，依然可以正常工作
static int singleTxWithDoneWhenTxCAN_tx_CAN_frame_with_done(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	iso_can_tp_L_Data_confirm(link, msg, 0);
	return 0;
}
TEST(singleTest, singleTxWithDoneWhenTxCAN)
{
	ULONGLONG tick_start = GetTickCount64();
	//print_enable = true;
	for (int TX_DLC = 8; TX_DLC <= TEST_DLC_MAX; TX_DLC++)
	{
		uint8_t len = dlc2len(TX_DLC) - 2;
		if (TX_DLC == 8) len = 7;
		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			testPrintf("test for TEST_DLC=%d  channel=%d", TX_DLC, i);
			init[i].TX_DLC = TX_DLC;
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));

			testPrintf("test for payload1");
			L_Data_request_hook = singleTxWithDoneWhenTxCAN_tx_CAN_frame_with_done;
			singleFrameTxTest(i, payload1, len);

			testPrintf("test for payload2");
			singleFrameTxTest(i, payload2, len-1);
			L_Data_request_hook = NULL;
		}
	}
}
//在发送CAN报文时被驱动拒绝，则会在下一次poll时继续尝试发送，直到发送超时
int singleTxCANFail_tx_CAN_frame_with_fail(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	return -1;
}
TEST(singleTest, singleTxCANFail)
{
	ULONGLONG tick_start = GetTickCount64();
	//print_enable = true;
	for (int TX_DLC = 8; TX_DLC <= TEST_DLC_MAX; TX_DLC++)
	{
		uint8_t len = dlc2len(TX_DLC) - 2;
		if (TX_DLC == 8) len = 7;
		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			testPrintf("test for TEST_DLC=%d  channel=%d", TX_DLC, i);
			init[i].TX_DLC = TX_DLC;
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));

			if (init[i].N_As > 2)
			{
				L_Data_request_hook = singleTxCANFail_tx_CAN_frame_with_fail;
				iso_can_tp_poll(&link[i], 0);

				init_last_tx_done_vars();
				init_last_tx_par_vars();
				LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload1, len));
				LONGS_EQUAL(1, last_tx_par[i].cnt);

				init_last_tx_par_vars();
				iso_can_tp_poll(&link[i], init[i].N_As - 2);
				LONGS_EQUAL(1, last_tx_par[i].cnt);
				iso_can_tp_poll(&link[i], init[i].N_As - 1);
				LONGS_EQUAL(2, last_tx_par[i].cnt);

				init_last_tx_par_vars();
				init_last_tx_done_vars();
				iso_can_tp_poll(&link[i], init[i].N_As + 1);
				LONGS_EQUAL(0, last_tx_par[i].cnt);
				LONGS_EQUAL(1, last_tx_done_par[i].cnt);
				LONGS_EQUAL(N_TIMEOUT_A, last_tx_done_par[i].par[0].error);

				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}
//在发送CAN报文时CAN驱动最终汇报发送失败，向上层汇报发送失败N_ERROR（注意：这里没有尝试重发，因为通常情况下CAN驱动都能发出去，如果发不出去，则很有可能是总线已经出现严重故障，为了避免对该故障产生进一步影响，此处选择停止发送而不是继续发送直到N_As超时）
TEST(singleTest, singleTxCANDriverFail)
{
	ULONGLONG tick_start = GetTickCount64();
	//print_enable = true;
	for (int TX_DLC = 8; TX_DLC <= TEST_DLC_MAX; TX_DLC++)
	{
		uint8_t len = dlc2len(TX_DLC) - 2;
		if (TX_DLC == 8) len = 7;
		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			testPrintf("test for TEST_DLC=%d  channel=%d", TX_DLC, i);
			init[i].TX_DLC = TX_DLC;
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));

			init_last_tx_par_vars();
			LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, len));
			LONGS_EQUAL(1, last_tx_par[i].cnt);

			init_last_tx_done_vars();
			LONGS_EQUAL(0, iso_can_tp_L_Data_confirm(&link[i], &last_tx_par[i].par[0].msg, -1));
			LONGS_EQUAL(1, last_tx_done_par[i].cnt);
			LONGS_EQUAL(N_ERROR, last_tx_done_par[i].par[0].error);
			LONGS_EQUAL(len, last_tx_done_par[i].par[0].size);
			POINTERS_EQUAL(payload, last_tx_done_par[i].par[0].payload);

			singleFrameTxTest(i, payload1, len);
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

//禁止发送报文时递归调用，测试以下路径：
// iso_can_tp_N_USData_request->调用底层CAN发送驱动L_Data_request->驱动在发送时调用iso_can_tp_L_Data_confirm->导致模块调用N_USData_confirm，应用层在发送完成通知函数中调用iso_can_tp_N_USData_request，则导致递归路径，此时代码在基于FIFO模块的支持下，应该返回ok并保存数据直到返回时重新发送报文
static int singleTxProhibitRecursiveCalls_new_request_cnt = 0;
int singleTxProhibitRecursiveCalls_tx_CAN_frame_with_done(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	iso_can_tp_L_Data_confirm(link, msg, 0);
	return 0;
}
void singleTxProhibitRecursiveCalls_N_USData_confirm_with_new_request(can_iso_tp_link_t_p link, const uint8_t payload[], uint32_t size, CAN_ISO_TP_RESAULT error)
{
	if (singleTxProhibitRecursiveCalls_new_request_cnt != 0)
	{
		singleTxProhibitRecursiveCalls_new_request_cnt--;
		uint8_t payload1[] = { 1,2 };
		iso_can_tp_N_USData_request(link, 0, payload1, sizeof(payload1));
	}
}
TEST(singleTest, singleTxProhibitRecursiveCalls)
{
	ULONGLONG tick_start = GetTickCount64();
	//print_enable = true;
	for (int TX_DLC = 8; TX_DLC <= TEST_DLC_MAX; TX_DLC++)
	{
		uint8_t len = dlc2len(TX_DLC) - 2;
		if (TX_DLC == 8) len = 7;
		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			testPrintf("test for TEST_DLC=%d  channel=%d", TX_DLC, i);
			init[i].TX_DLC = TX_DLC;
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));

			//每个通道重发次数不同
			int retry_cnt = i + 1;
			singleTxProhibitRecursiveCalls_new_request_cnt = retry_cnt;
			L_Data_request_hook = singleTxProhibitRecursiveCalls_tx_CAN_frame_with_done;
			N_USData_confirm_hook = singleTxProhibitRecursiveCalls_N_USData_confirm_with_new_request;

			init_last_tx_par_vars();
			init_last_tx_done_vars();
			LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload1, len));

			LONGS_EQUAL(retry_cnt + 1, last_tx_par[i].cnt);
			LONGS_EQUAL(retry_cnt + 1, last_tx_done_par[i].cnt);
			for (int j = 0; j < last_tx_done_par[i].cnt; j++)
			{
				LONGS_EQUAL(N_OK, last_tx_done_par[i].par[j].error);
			}

			L_Data_request_hook = NULL;
			N_USData_confirm_hook = NULL;
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

//正确初始化后接收单帧，并过滤不正确的单帧
TEST(singleTest, singleRx)
{
	ULONGLONG tick_start = GetTickCount64();
	//print_enable = true;
	for (int TEST_DLC = 2; TEST_DLC <= TEST_DLC_MAX; TEST_DLC++)
	{
		uint8_t len = dlc2len(TEST_DLC) - 2;
		if (TEST_DLC <= 8) len = dlc2len(TEST_DLC) - 1;
		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			testPrintf("test for TEST_DLC=%d  channel=%d", TEST_DLC, i);
			init[i].TX_DLC = TEST_DLC;
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));

			if (TEST_DLC <= 8)
			{
				for (uint8_t rx_len = 1; rx_len <= len; rx_len++)
				{
					testPrintf("test for rx_len=%d  ", rx_len);
					payload[0] = rx_len;
					singleFrameRxTest(i, payload, TEST_DLC);
					singleFrameRxTest(i, 1, payload, TEST_DLC);
				}

				//不应接受第一字节为0的报文
				{
					payload[0] = 0;
					struct CAN_msg msg;
					msg.id = init[i].rx_id;
					msg.dlc = TEST_DLC;
					memcpy(msg.data, payload, dlc2len(TEST_DLC));
					init_last_tp_recieve_vars();
					iso_can_tp_L_Data_indication(&link[i], &msg);
					LONGS_EQUAL(0, last_rx_par[i].cnt);
				}
				{
					payload[0] = 0;
					struct CAN_msg msg;
					msg.id = init[i].funtion_id;
					msg.dlc = TEST_DLC;
					memcpy(msg.data, payload, dlc2len(TEST_DLC));
					init_last_tp_recieve_vars();
					iso_can_tp_L_Data_indication(&link[i], &msg);
					LONGS_EQUAL(0, last_rx_par[i].cnt);
				}

				//不应接受第一字节低4bit太长的报文
				for (uint16_t rx_len = len + 1; rx_len <= 0xf; rx_len++)
				{
					{
						payload[0] = (uint8_t)rx_len;
						struct CAN_msg msg;
						msg.id = init[i].rx_id;
						msg.dlc = TEST_DLC;
						memcpy(msg.data, payload, dlc2len(TEST_DLC));
						init_last_tp_recieve_vars();
						iso_can_tp_L_Data_indication(&link[i], &msg);
						LONGS_EQUAL(0, last_rx_par[i].cnt);
					}
					{
						payload[0] = (uint8_t)rx_len;
						struct CAN_msg msg;
						msg.id = init[i].funtion_id;
						msg.dlc = TEST_DLC;
						memcpy(msg.data, payload, dlc2len(TEST_DLC));
						init_last_tp_recieve_vars();
						iso_can_tp_L_Data_indication(&link[i], &msg);
						LONGS_EQUAL(0, last_rx_par[i].cnt);
					}
				}
			}
			else {
				for (uint8_t rx_len = 1; rx_len <= len; rx_len++)
				{
					testPrintf("test for rx_len=%d  ", rx_len);
					payload[0] = 0;
					payload[1] = rx_len;
					singleFrameRxTest(i, payload, TEST_DLC);
					singleFrameRxTest(i, 1, payload, TEST_DLC);
				}

				//不应接受第二字节为0的报文
				{
					payload[1] = 0;
					struct CAN_msg msg;
					msg.id = init[i].rx_id;
					msg.dlc = TEST_DLC;
					memcpy(msg.data, payload, dlc2len(TEST_DLC));
					init_last_tp_recieve_vars();
					iso_can_tp_L_Data_indication(&link[i], &msg);
					LONGS_EQUAL(0, last_rx_par[i].cnt);
				}
				{
					payload[1] = 0;
					struct CAN_msg msg;
					msg.id = init[i].funtion_id;
					msg.dlc = TEST_DLC;
					memcpy(msg.data, payload, dlc2len(TEST_DLC));
					init_last_tp_recieve_vars();
					iso_can_tp_L_Data_indication(&link[i], &msg);
					LONGS_EQUAL(0, last_rx_par[i].cnt);
				}

				//不应接受第一字节低4bit非0的报文
				for (uint8_t low_4bit = 1; low_4bit <= 0xf; low_4bit++)
				{
					{
						payload[0] = low_4bit;
						payload[1] = len;
						struct CAN_msg msg;
						msg.id = init[i].rx_id;
						msg.dlc = TEST_DLC;
						memcpy(msg.data, payload, dlc2len(TEST_DLC));
						init_last_tp_recieve_vars();
						iso_can_tp_L_Data_indication(&link[i], &msg);
						LONGS_EQUAL(0, last_rx_par[i].cnt);
					}
					{
						payload[1] = low_4bit;
						payload[1] = len;
						struct CAN_msg msg;
						msg.id = init[i].funtion_id;
						msg.dlc = TEST_DLC;
						memcpy(msg.data, payload, dlc2len(TEST_DLC));
						init_last_tp_recieve_vars();
						iso_can_tp_L_Data_indication(&link[i], &msg);
						LONGS_EQUAL(0, last_rx_par[i].cnt);
					}
				}

				//不应接受报文数据长度长于DLC的报文
				for (uint16_t rx_len = len + 1; rx_len <= 0xff; rx_len++)
				{
					{
						payload[0] = 0;
						payload[1] = (uint8_t)rx_len;
						struct CAN_msg msg;
						msg.id = init[i].rx_id;
						msg.dlc = TEST_DLC;
						memcpy(msg.data, payload, dlc2len(TEST_DLC));
						init_last_tp_recieve_vars();
						iso_can_tp_L_Data_indication(&link[i], &msg);
						LONGS_EQUAL(0, last_rx_par[i].cnt);
					}
					{
						payload[0] = 0;
						payload[1] = (uint8_t)rx_len;
						struct CAN_msg msg;
						msg.id = init[i].funtion_id;
						msg.dlc = TEST_DLC;
						memcpy(msg.data, payload, dlc2len(TEST_DLC));
						init_last_tp_recieve_vars();
						iso_can_tp_L_Data_indication(&link[i], &msg);
						LONGS_EQUAL(0, last_rx_par[i].cnt);
					}
				}
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}
//正确初始化后接收ID不匹配单帧，不汇报
TEST(singleTest, singleRxWrongID)
{
	ULONGLONG tick_start = GetTickCount64();
	//print_enable = true;
	for (int TEST_DLC = 2; TEST_DLC <= TEST_DLC_MAX; TEST_DLC++)
	{
		uint8_t len = dlc2len(TEST_DLC) - 2;
		if (TEST_DLC <= 8) len = dlc2len(TEST_DLC) - 1;
		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			testPrintf("test for TEST_DLC=%d  channel=%d", TEST_DLC, i);
			init[i].TX_DLC = TEST_DLC;
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));

			struct CAN_msg msg;
			if (TEST_DLC <= 8)
			{
				payload2[0] = 1;
			}
			else {
				payload2[0] = 0;
				payload2[1] = 3;
			}
			msg.id = init[i].rx_id;
			memcpy(msg.data, payload2, dlc2len(TEST_DLC));

			//id 类型不匹配
			msg.id = init[i].rx_id;
			msg.id.isExt = !init[i].rx_id.isExt;
			init_last_tp_recieve_vars();
			iso_can_tp_L_Data_indication(&link[i], &msg);
			LONGS_EQUAL(0, last_rx_par[i].cnt);
			//id 值不匹配
			msg.id = init[i].rx_id;
			msg.id.id = init[i].rx_id.id + 1;
			init_last_tp_recieve_vars();
			iso_can_tp_L_Data_indication(&link[i], &msg);
			LONGS_EQUAL(0, last_rx_par[i].cnt);
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}
/*验证模块在不支持CANFD时可以正确抛弃CANFD报文*/
//测试步骤：
//1. 在不支持CANFD模式下编译运行
//1. 收到CANFD单帧报文
//期待结果：
//节点抛弃该报文并报错
#ifndef SUPPORT_CAN_FD
TEST(singleTest, DoNotSupportCANFD)
{
	ULONGLONG tick_start = GetTickCount64();
	//print_enable = true;
	for (int TEST_DLC = 2; TEST_DLC <= TEST_DLC_MAX; TEST_DLC++)
	{
		uint8_t len = dlc2len(TEST_DLC) - 2;
		if (TEST_DLC <= 8) len = dlc2len(TEST_DLC) - 1;
		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			testPrintf("test for TEST_DLC=%d  channel=%d", TEST_DLC, i);
			init[i].TX_DLC = TEST_DLC;
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));

			for (int isCANFD= 0; isCANFD < 2; isCANFD++)
			{
				if ((isCANFD != 0) || (TEST_DLC > 8))
				{
					struct CAN_msg msg;
					if (TEST_DLC <= 8)
					{
						payload2[0] = 1;
					}
					else {
						payload2[0] = 0;
						payload2[1] = 3;
					}
					msg.id = init[i].rx_id;
					memcpy(msg.data, payload2, 8);  
					msg.id.isCANFD = isCANFD;
					msg.dlc = TEST_DLC;
					init_last_tp_recieve_vars();
					iso_can_tp_L_Data_indication(&link[i], &msg);
					LONGS_EQUAL(0, last_rx_par[i].cnt);
				}
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}
#endif