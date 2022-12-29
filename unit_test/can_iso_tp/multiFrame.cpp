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

static void print_debug_info(const char* msg)
{
	testPrintf(msg);
}

TEST_GROUP(multiTest)
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
			memset(&init[i], 0, sizeof(struct can_iso_tp_init_t));
			init[i].rx_id.id = i * 50;
			init[i].rx_id.isExt = 0;
			init[i].rx_id.isCANFD = 0;
			init[i].rx_id.isRemote = 0;
			init[i].tx_id.id = i * 50 + 1;
			init[i].tx_id.isExt = 0;
			init[i].tx_id.isCANFD = 0;
			init[i].tx_id.isRemote = 0;
			init[i].funtion_id.id = i * 50 + 2;
			init[i].funtion_id.isExt = 0;
			init[i].funtion_id.isCANFD = 0;
			init[i].funtion_id.isRemote = 0;
			init[i].N_Ar = 1000;
			init[i].N_As = 900;
			init[i].N_Bs = 800;
			init[i].N_Cr = 700;
			init[i].N_WFTmax = 10;
			init[i].L_Data_request = L_Data_request;
			init[i].N_USData_indication = N_USData_indication;
			init[i].N_USData_confirm = N_USData_confirm;
			init[i].rx_buff = rx_buff;
			init[i].rx_buff_len = sizeof(rx_buff);
			init[i].TX_DLC = 8;
			init[i].frame_pad = 0x77;
			init[i].print_debug = print_debug_info;
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));
		}
	}
	void teardown()
	{
		destory_all_modules();
	}
};

/*��֤ģ���������������֡*/
//���Բ��裺
//1. �������Ͷ�֡������Ϊ8-4095�ֽ�����
//�ڴ������
//�ڵ㷢����֡����û���յ����ص�����²���������
TEST(multiTest, nomralTxFF)
{
	ULONGLONG tick_start = GetTickCount64();
	//print_enable = true;
	for (int TX_DLC = 8; TX_DLC <= TEST_DLC_MAX; TX_DLC++)
	{
		for (int i = 0; i < TEST_CHANNEL_NUM; i++)
		{
			testPrintf("test for TEST_DLC=%d  channel=%d", TX_DLC, i);
			init_last_tx_par_vars();
			init_last_tp_recieve_vars();
			init_last_tx_done_vars();

			init[i].TX_DLC = TX_DLC;
			LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));
			uint64_t len;
			uint8_t len_start;
			if (TX_DLC == 8)
			{
				len_start = dlc2len(TX_DLC) - 1 + 1;
			}
			else {
				len_start = dlc2len(TX_DLC) - 2 + 1;
			}
			uint64_t len_inc = 1;
			for (len = len_start; len <= 0xffffffff; len+=0xfff/4000 + len_inc)
			{
				if (len_inc > 0xfff) len_inc = 0xffffffff / 100;
				testPrintf("test for len=%d", len);

				if (TX_DLC <= 8) {
					init[i].tx_id.isCANFD = 0;
				}
				else {
					init[i].tx_id.isCANFD = 1;
				}
				LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));
				init_last_tx_par_vars();
				LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, (uint32_t)len));
				LONGS_EQUAL(1, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[0].msg.id.isExt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[0].msg.id.id);
				LONGS_EQUAL(TX_DLC, last_tx_par[i].par[0].msg.dlc);

				if (len <= 0xfff)
				{
					//N_PCItype = 1
					LONGS_EQUAL(0x10, last_tx_par[i].par[0].msg.data[0] & 0xf0);
					//FF_DL
					LONGS_EQUAL(len, (last_tx_par[i].par[0].msg.data[0] & 0xf) * 256 + last_tx_par[i].par[0].msg.data[1]);
					LONGS_EQUAL(0, memcmp(&last_tx_par[i].par[0].msg.data[2], payload, dlc2len(TX_DLC) - 2));
				}
				else {
					len_inc = 0xffffffff / 20;
					//N_PCItype = 1
					LONGS_EQUAL(0x10, last_tx_par[i].par[0].msg.data[0]);
					LONGS_EQUAL(0, last_tx_par[i].par[0].msg.data[1]);
					//FF_DL
					LONGS_EQUAL(len, (uint32_t)(last_tx_par[i].par[0].msg.data[2]) * 256 * 65536 
						           + (uint32_t)(last_tx_par[i].par[0].msg.data[3])* 65536
									+ (uint32_t)(last_tx_par[i].par[0].msg.data[4]) * 256
									+ (uint32_t)(last_tx_par[i].par[0].msg.data[5])
						           );
					LONGS_EQUAL(0, memcmp(&last_tx_par[i].par[0].msg.data[6], payload, dlc2len(TX_DLC) - 6));
				}
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[0].msg.id.isExt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[0].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isCANFD, last_tx_par[i].par[0].msg.id.isCANFD);
				LONGS_EQUAL(0, last_tx_par[i].par[0].msg.id.isRemote);

				iso_can_tp_poll(&link[i], 0);
				LONGS_EQUAL(1, last_tx_par[i].cnt);
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}
/*��֤ģ������������Ͷ�֡���ݣ����٣�*/
//���Բ��裺
//1. �������Ͷ�֡������Ϊ8-4095�ֽ�����
//2. �Է����ء��������͡������ر���
//3. ���Թ�������L_Data_request�����ڵ���iso_can_tp_L_Data_confirm
//�ڴ������
//�ڵ���Խ���֡�������յ����غ��ʣ�µı���һ�η���
//���͹����в�������ֵݹ���Ϊ������iso_can_tp_L_Data_indication�յ�����->����L_Data_request����CF����->���ͺ����ڲ�����iso_can_tp_L_Data_confirmȷ�Ϸ������->����L_Data_request������һ֡CF���ģ���ʱ�������ͣ���Ҫ������һ�ε���L_Data_request�еĺ������ͱ��ģ�
int nomralTxFast_tx_CAN_frame_with_done(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	iso_can_tp_L_Data_confirm(link, msg, 0);
	return 0;
}
TEST(multiTest, nomralTxFast)
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
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff/1000 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 10;
				testPrintf("test for len %d.\n", len);
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				//-----------------------------------------------------------------
				L_Data_request_hook = nomralTxFast_tx_CAN_frame_with_done;

				init_last_tx_par_vars();
				LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, (uint32_t)len));
				LONGS_EQUAL(1, last_tx_par[i].cnt);

				struct CAN_msg flowControlMsg;
				flowControlMsg.dlc = TX_DLC;
				flowControlMsg.id = init[i].rx_id;
				memset(flowControlMsg.data, 0, dlc2len(flowControlMsg.dlc));
				flowControlMsg.data[0] = 0x30;
				iso_can_tp_L_Data_indication(&link[i], &flowControlMsg);

				uint32_t txFrameNum;
				uint64_t txIndex;
				uint16_t cf_tx_data_len = (dlc2len(TX_DLC) - 1);
				{
					uint16_t ff_tx_data_len;
					if (len <= 0xfff) {
						ff_tx_data_len = dlc2len(TX_DLC) - 2;
					}
					else {
						ff_tx_data_len = dlc2len(TX_DLC) - 6;
					}
					txIndex = ff_tx_data_len;
					txFrameNum = 1 + (int)((len - ff_tx_data_len) / cf_tx_data_len) + (int)!!((len - ff_tx_data_len) % cf_tx_data_len);
				}
				uint8_t SN = 0;
				LONGS_EQUAL(txFrameNum, last_tx_par[i].cnt);
				for (uint32_t j = 1; j < txFrameNum; j++)
				{
					//���������֡����ȷ��
					//testPrintf("test for j %dth cf.\n", j);
					SN++;
					LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[j].msg.id.id);
					LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[j].msg.id.isExt);
					LONGS_EQUAL(0x20, last_tx_par[i].par[j].msg.data[0] & 0xf0);
					LONGS_EQUAL(SN & 0xf, last_tx_par[i].par[j].msg.data[0] & 0xf);
					uint32_t checkLen = (uint32_t)(len - txIndex);
					if (checkLen > cf_tx_data_len)
					{
						checkLen = cf_tx_data_len;
					}
					LONGS_EQUAL(0, memcmp(&last_tx_par[i].par[j].msg.data[1], &payload[txIndex], checkLen));
					txIndex += checkLen;
				}
				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}
/*��֤ģ������������Ͷ�֡���ݣ����٣�*/
//���Բ��裺
//1. �������Ͷ�֡������Ϊ8-4095�ֽ�����
//2. �Է����ء��������͡������ر���
//3. ���Թ������ڲ������庯���ڵ���iso_can_tp_L_Data_confirm
//�ڴ������
//�ڵ���Խ���֡������ÿһ��iso_can_tp_L_Data_confirm�з�����һ֡CF���ģ��������б��İ���ISOҪ����
TEST(multiTest, nomralTxSlow)
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
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff / 200 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 35;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------

				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, (uint32_t)len));
				LONGS_EQUAL(1, last_tx_par[i].cnt);
				iso_can_tp_L_Data_confirm(&link[i], &last_tx_par[i].par[0].msg, 0);

				struct CAN_msg flowControlMsg;
				flowControlMsg.dlc = 8;
				flowControlMsg.id = init[i].rx_id;
				memset(flowControlMsg.data, 0, flowControlMsg.dlc);
				flowControlMsg.data[0] = 0x30;
				iso_can_tp_L_Data_indication(&link[i], &flowControlMsg);

				uint32_t txFrameNum;
				uint64_t txIndex;
				uint16_t cf_tx_data_len = (dlc2len(TX_DLC) - 1);
				{
					uint16_t ff_tx_data_len;
					if (len <= 0xfff) {
						ff_tx_data_len = dlc2len(TX_DLC) - 2;
					}
					else {
						ff_tx_data_len = dlc2len(TX_DLC) - 6;
					}
					txIndex = ff_tx_data_len;
					txFrameNum = 1 + (int)((len - ff_tx_data_len) / cf_tx_data_len) + (int)!!((len - ff_tx_data_len) % cf_tx_data_len);
				}
				uint8_t SN = 0;

				for (uint32_t j = 1; j < txFrameNum; j++)
				{
					//���������֡����ȷ��
					testPrintf("test for j %d.\n", j);
					SN++;
					LONGS_EQUAL(j + 1, last_tx_par[i].cnt);
					LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[j].msg.id.id);
					LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[j].msg.id.isExt);
					LONGS_EQUAL(0x20, last_tx_par[i].par[j].msg.data[0] & 0xf0);
					LONGS_EQUAL(SN & 0xf, last_tx_par[i].par[j].msg.data[0] & 0xf);
					uint32_t checkLen = (uint32_t)(len - txIndex);
					if (checkLen > cf_tx_data_len)
					{
						checkLen = cf_tx_data_len;
					}
					LONGS_EQUAL(0, memcmp(&last_tx_par[i].par[j].msg.data[1], &payload[txIndex], checkLen));
					txIndex += checkLen;

					iso_can_tp_L_Data_confirm(&link[i], &last_tx_par[i].par[j].msg, 0);
				}
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}
/*��֤ģ��������������͹������յ���������ϢFS��������WAIT��Ϣ*/
//����������
//1. ���Թ�������L_Data_request�����ڵ���iso_can_tp_L_Data_confirm
//���Բ��裺
//1. �������Ͷ�֡������Ϊ��256�ֽ�����
//2. �Է�����������ϢFS��������WAIT�����ر���
//3. �ھ�����һ�η������ر�����δ����N_Bs������µ���iso_can_tp_poll��������������Ϊ
//4. �ظ�����2-3ֱ�����͵�WAIT��������Ϊ������N_WFTmax-1
//5. ���͡��������͡������ر���
//6. ��������Ϊ
//�ڴ������
//1. ��3���������������������
//2. ��6������������������̷���ʣ������
int txFcWait_tx_CAN_frame_with_done(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	iso_can_tp_L_Data_confirm(link, msg, 0);
	return 0;
}
TEST(multiTest, txFcWait)
{
	ULONGLONG tick_start = GetTickCount64();
	//print_enable = true;
	L_Data_request_hook = txFcWait_tx_CAN_frame_with_done;
	for (int i = 0; i < TEST_CHANNEL_NUM; i++)
	{
		testPrintf("test for channel %d.\n", i);
		if (init[i].N_WFTmax == 0)
			continue;
		testPrintf("step1 �������Ͷ�֡������Ϊ��256�ֽ�����\n");
		iso_can_tp_poll(&link[i], 0);
		init_last_tx_par_vars();
		int len = 256;
		if (len > sizeof(payload))len = sizeof(payload);
		LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, len));
		for (uint32_t j = 0; j < (uint32_t)(init[i].N_WFTmax - 1); j++)
		{
			testPrintf("step2 �Է�����������ϢFS��������WAIT�����ر���\n");
			struct CAN_msg flowControlMsg;
			flowControlMsg.dlc = 8;
			flowControlMsg.id = init[i].rx_id;
			memset(flowControlMsg.data, 0, flowControlMsg.dlc);
			flowControlMsg.data[0] = 0x31;
			iso_can_tp_L_Data_indication(&link[i], &flowControlMsg);
			testPrintf("step3 �ھ�����һ�η������ر�����δ����N_Bs������µ���iso_can_tp_poll��������������Ϊ\n");
			if (init[i].N_Bs > 3)
			{
				iso_can_tp_poll(&link[i], init[i].N_Bs - 3);
				LONGS_EQUAL(1, last_tx_par[i].cnt);
			}
			if (j == 0)
			{
				testPrintf("step4 �ظ�����2-3ֱ�����͵�WAIT��������Ϊ������N_WFTmax(%d)-1\n", init[i].N_WFTmax);
			}
		}
		{
			testPrintf("step5 ���͡��������͡������ر���\n");
			struct CAN_msg flowControlMsg;
			flowControlMsg.dlc = 8;
			flowControlMsg.id = init[i].rx_id;
			memset(flowControlMsg.data, 0, flowControlMsg.dlc);
			flowControlMsg.data[0] = 0x30;
			iso_can_tp_L_Data_indication(&link[i], &flowControlMsg);
		}
		testPrintf("step6 ��������Ϊ\n");
		uint32_t txFrameNum = 1 + ((len - 6) / 7) + !!((len - 6) % 7);
		uint8_t SN = 0;
		uint16_t txIndex = 6;
		LONGS_EQUAL(txFrameNum, last_tx_par[i].cnt);
		for (uint32_t j = 1; j < txFrameNum; j++)
		{
			//���������֡����ȷ��
			testPrintf("test for j %d.\n", j);
			SN++;
			LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[j].msg.id.id);
			LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[j].msg.id.isExt);
			LONGS_EQUAL(0x20, last_tx_par[i].par[j].msg.data[0] & 0xf0);
			LONGS_EQUAL(SN & 0xf, last_tx_par[i].par[j].msg.data[0] & 0xf);
			uint32_t checkLen = len - txIndex;
			if (checkLen > 7)
			{
				checkLen = 7;
			}
			LONGS_EQUAL(0, memcmp(&last_tx_par[i].par[j].msg.data[1], &payload[txIndex], checkLen));
			txIndex += checkLen;

			iso_can_tp_L_Data_confirm(&link[i], &last_tx_par[i].par[j].msg, 0);
		}

	}
	L_Data_request_hook = NULL;
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}
/*��֤ģ��������������͹������յ���������ϢFS��������WAIT��Ϣ*/
//����������
//1. ���Թ�������L_Data_request�����ڵ���iso_can_tp_L_Data_confirm
//���Բ��裺
//1. �������Ͷ�֡������Ϊ��256�ֽ�����
//2. �Է�����������ϢFS��������WAIT�����ر���
//3. �ھ�����һ�η������ر�����δ����N_Bs������µ���iso_can_tp_poll��������������Ϊ
//4. �ظ�����2-3ֱ�����͵�WAIT��������Ϊ������N_WFTmax
//5. ���͡��������͡������ر���
//6. ��������Ϊ
//�ڴ������
//2. ��6����������ٷ�������
int txFcWaitMax_tx_CAN_frame_with_done(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	iso_can_tp_L_Data_confirm(link, msg, 0);
	return 0;
}
TEST(multiTest, txFcWaitMax)
{
	ULONGLONG tick_start = GetTickCount64();
	uint8_t payload[256];
	//print_enable = true;
	for (int i = 0; i < sizeof(payload); i++)
	{
		payload[i] = (uint8_t)i;
	}

	for (int i = 0; i < TEST_CHANNEL_NUM; i++)
	{
		testPrintf("test for channel %d.\n", i);
		L_Data_request_hook = txFcWaitMax_tx_CAN_frame_with_done;
		if (init[i].N_WFTmax == 0)
			continue;
		testPrintf("step1 �������Ͷ�֡������Ϊ��256�ֽ�����\n");
		iso_can_tp_poll(&link[i], 0);
		init_last_tx_par_vars();
		LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, sizeof(payload)));
		for (uint32_t j = 0; j < (init[i].N_WFTmax); j++)
		{
			testPrintf("step2 �Է�����������ϢFS��������WAIT�����ر���\n");
			struct CAN_msg flowControlMsg;
			flowControlMsg.dlc = 8;
			flowControlMsg.id = init[i].rx_id;
			memset(flowControlMsg.data, 0, flowControlMsg.dlc);
			flowControlMsg.data[0] = 0x31;
			iso_can_tp_L_Data_indication(&link[i], &flowControlMsg);
			testPrintf("step3 �ھ�����һ�η������ر�����δ����N_Bs������µ���iso_can_tp_poll��������������Ϊ\n");
			if (init[i].N_Bs > 3)
			{
				iso_can_tp_poll(&link[i], init[i].N_Bs - 3);
				LONGS_EQUAL(1, last_tx_par[i].cnt);
			}
			if (j == 0)
			{
				testPrintf("step4 �ظ�����2-3ֱ�����͵�WAIT��������Ϊ������N_WFTmax(%d)\n", init[i].N_WFTmax);
			}
		}
		{
			testPrintf("step5 ���͡��������͡������ر���\n");
			struct CAN_msg flowControlMsg;
			flowControlMsg.dlc = 8;
			flowControlMsg.id = init[i].rx_id;
			memset(flowControlMsg.data, 0, flowControlMsg.dlc);
			flowControlMsg.data[0] = 0x30;
			iso_can_tp_L_Data_indication(&link[i], &flowControlMsg);
		}
		testPrintf("step6 ��������Ϊ\n");
		LONGS_EQUAL(1, last_tx_par[i].cnt);
		L_Data_request_hook = NULL;
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

/*��֤ģ��������������͹������յ���������ϢFS��������Overflow��Ϣ*/
//����������
//1. ���Թ�������L_Data_request�����ڵ���iso_can_tp_L_Data_confirm
//���Բ��裺
//1. �������Ͷ�֡������Ϊ��8-4095�ֽ�����
//2. �Է�����������ϢFS��������Overflow�����ر���
//3. ��������Ϊ
//�ڴ������
//1. ��3�������ֹͣ���ͱ��ģ������ϲ�㱨����ʧ����Ϣ
int txFcOverflow_tx_CAN_frame_with_done(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	iso_can_tp_L_Data_confirm(link, msg, 0);
	return 0;
}
TEST(multiTest, txFcOverflow)
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
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff/2000 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 40;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------
				L_Data_request_hook = nomralTxFast_tx_CAN_frame_with_done;
				LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, (uint32_t)len));
				struct CAN_msg flowControlMsg;
				flowControlMsg.dlc = 8;
				flowControlMsg.id = init[i].rx_id;
				memset(flowControlMsg.data, 0, flowControlMsg.dlc);
				flowControlMsg.data[0] = 0x32;
				iso_can_tp_L_Data_indication(&link[i], &flowControlMsg);

				LONGS_EQUAL(1, last_tx_par[i].cnt);
				LONGS_EQUAL(1, last_tx_done_par[i].cnt);
				LONGS_EQUAL(N_BUFFER_OVFLW, last_tx_done_par[i].par[0].error);
				LONGS_EQUAL(len, last_tx_done_par[i].par[0].size);
				LONGS_EQUAL(0, memcmp(last_tx_done_par[i].par[0].payload, &payload[0], (size_t)len));
				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

/*��֤ģ��������������͹������յ���������ϢBS��Ϣ*/
//����������
//1. ���Թ�������L_Data_request�����ڵ���iso_can_tp_L_Data_confirm
//���Բ��裺
//1. �������Ͷ�֡������Ϊ��4095�ֽ�����
//2. ����������ϢBSֵΪ1-255
//3. ���ÿһ��BSֵ�����������Ϊ
//�ڴ������
//1. �������ISO�涨������BS�����ĺ�ֹͣ���Ͳ��ȴ��µ�������Ϣ
int txBSFrames_tx_CAN_frame_with_done(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	iso_can_tp_L_Data_confirm(link, msg, 0);
	return 0;
}
TEST(multiTest, txBSFrames)
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
			//for (uint64_t len = len_start; len < sizeof(payload); len += 400 + len_inc)
			uint64_t len = sizeof(payload);
			if(len >= len_start)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 10;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------
				L_Data_request_hook = txBSFrames_tx_CAN_frame_with_done;
				for (int BS = 1; BS < 255; BS += 4)
				{
					init_last_tx_par_vars();
					init_last_tp_recieve_vars();
					init_last_tx_done_vars();

					testPrintf("test for bs %d.\n", BS);
					LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, (uint32_t)len));

					uint32_t txFrameNum;
					uint64_t txIndex;
					uint16_t cf_tx_data_len = (dlc2len(TX_DLC) - 1);
					{
						uint16_t ff_tx_data_len;
						if (len <= 0xfff) {
							ff_tx_data_len = dlc2len(TX_DLC) - 2;
						}
						else {
							ff_tx_data_len = dlc2len(TX_DLC) - 6;
						}
						txIndex = ff_tx_data_len;
						txFrameNum = 1 + (int)((len - ff_tx_data_len) / cf_tx_data_len) + (int)!!((len - ff_tx_data_len) % cf_tx_data_len);
					}
					uint8_t SN = 0;
					uint32_t frameIndex = 1;
					for (uint32_t j = 0; j < ((txFrameNum - 1) / BS); j++)
					{
						struct CAN_msg flowControlMsg;
						flowControlMsg.dlc = 8;
						flowControlMsg.id = init[i].rx_id;
						memset(flowControlMsg.data, 0, flowControlMsg.dlc);
						flowControlMsg.data[0] = 0x30;
						flowControlMsg.data[1] = BS;
						iso_can_tp_L_Data_indication(&link[i], &flowControlMsg);

						for (int k = 0; k < BS; k++)
						{
							SN++;
							LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[frameIndex].msg.id.id);
							LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[frameIndex].msg.id.isExt);
							LONGS_EQUAL(0x20, last_tx_par[i].par[frameIndex].msg.data[0] & 0xf0);
							LONGS_EQUAL(SN & 0xf, last_tx_par[i].par[frameIndex].msg.data[0] & 0xf);
							uint32_t checkLen = (uint32_t)(len - txIndex);
							if (checkLen > cf_tx_data_len)
							{
								checkLen = cf_tx_data_len;
							}
							LONGS_EQUAL(0, memcmp(&last_tx_par[i].par[frameIndex].msg.data[1], &payload[txIndex], checkLen));
							frameIndex++;
							txIndex += checkLen;
						}
						LONGS_EQUAL(frameIndex, last_tx_par[i].cnt);
					}
					uint8_t frameRemain = (txFrameNum - 1) % BS;
					if (0 != frameRemain)
					{
						struct CAN_msg flowControlMsg;
						flowControlMsg.dlc = 8;
						flowControlMsg.id = init[i].rx_id;
						memset(flowControlMsg.data, 0, flowControlMsg.dlc);
						flowControlMsg.data[0] = 0x30;
						flowControlMsg.data[1] = BS;
						iso_can_tp_L_Data_indication(&link[i], &flowControlMsg);

						for (int k = 0; k < frameRemain; k++)
						{
							SN++;
							LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[frameIndex].msg.id.id);
							LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[frameIndex].msg.id.isExt);
							LONGS_EQUAL(0x20, last_tx_par[i].par[frameIndex].msg.data[0] & 0xf0);
							LONGS_EQUAL(SN & 0xf, last_tx_par[i].par[frameIndex].msg.data[0] & 0xf);
							uint32_t checkLen = (uint32_t)(len - txIndex);
							if (checkLen > cf_tx_data_len)
							{
								checkLen = cf_tx_data_len;
							}
							LONGS_EQUAL(0, memcmp(&last_tx_par[i].par[frameIndex].msg.data[1], &payload[txIndex], checkLen));
							for (int m = 1+ checkLen; m < dlc2len(TX_DLC); m++)
							{
								LONGS_EQUAL(init[i].frame_pad, last_tx_par[i].par[frameIndex].msg.data[m]);
							}
							frameIndex++;
							txIndex += checkLen;
						}
						LONGS_EQUAL(txFrameNum, last_tx_par[i].cnt);
					}
					LONGS_EQUAL(1, last_tx_done_par[i].cnt);
					LONGS_EQUAL(len, last_tx_done_par[i].par[0].size);
					LONGS_EQUAL(N_OK, last_tx_done_par[i].par[0].error);
					LONGS_EQUAL(0, memcmp(last_tx_done_par[i].par[0].payload, &payload[0], (size_t)len));
				}
				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

/*��֤ģ��������������͹������յ���������ϢSTmin��Ϣ*/
//����������
//1. ���Թ�������L_Data_request�����ڵ���iso_can_tp_L_Data_confirm
//���Բ��裺
//1. �������Ͷ�֡������Ϊ��4095�ֽ�����
//2. ����������ϢSTminֵΪ���������ֵ
//3. ���ÿһ��STminֵ�����������Ϊ
//�ڴ������
//1. �������ISO�涨��������������֮��ļ����������STmin�������ʱ��ֵ����poll���ڴ����STmin�������ʱ��ֵʱ�����������ڣ�
int txStmin_tx_CAN_frame_with_done(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	iso_can_tp_L_Data_confirm(link, msg, 0);
	return 0;
}
TEST(multiTest, txStmin)
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
			//for (uint64_t len = len_start; len < sizeof(payload); len += 400 + len_inc)
			uint64_t len = sizeof(payload);
			if (len >= len_start)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 10;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------

				L_Data_request_hook = txBSFrames_tx_CAN_frame_with_done;
				for (int Stmin = 1; Stmin < 0xff; Stmin += 5)
				{
					int waitTime = Stmin;
					if (Stmin > 0x7f)
						waitTime = 1;
					int poll_time = 0;
					testPrintf("test for channel %d and Stmin %d.\n", i, Stmin);
					init_last_tx_par_vars();
					init_last_tx_done_vars();
					iso_can_tp_poll(&link[i], poll_time);
					LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, (uint32_t)len));
					LONGS_EQUAL(1, last_tx_par[i].cnt);

					uint32_t txFrameNum;
					uint64_t txIndex;
					uint16_t cf_tx_data_len = (dlc2len(TX_DLC) - 1);
					{
						uint16_t ff_tx_data_len;
						if (len <= 0xfff) {
							ff_tx_data_len = dlc2len(TX_DLC) - 2;
						}
						else {
							ff_tx_data_len = dlc2len(TX_DLC) - 6;
						}
						txIndex = ff_tx_data_len;
						txFrameNum = 1 + (int)((len - ff_tx_data_len) / cf_tx_data_len) + (int)!!((len - ff_tx_data_len) % cf_tx_data_len);
					}
					uint8_t SN = 0;
					uint32_t frameIndex = 1;

					struct CAN_msg flowControlMsg;
					flowControlMsg.dlc = TX_DLC;
					flowControlMsg.id = init[i].rx_id;
					memset(flowControlMsg.data, 0, flowControlMsg.dlc);
					flowControlMsg.data[0] = 0x30;
					flowControlMsg.data[1] = 0;
					flowControlMsg.data[2] = Stmin;
					iso_can_tp_L_Data_indication(&link[i], &flowControlMsg);

					for (uint32_t k = 1; k < txFrameNum; k++)
					{
						testPrintf("test for k %d.\n", k);
						LONGS_EQUAL(1 + k, last_tx_par[i].cnt);
						SN++;
						LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[frameIndex].msg.id.id);
						LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[frameIndex].msg.id.isExt);
						LONGS_EQUAL(0x20, last_tx_par[i].par[frameIndex].msg.data[0] & 0xf0);
						LONGS_EQUAL(SN & 0xf, last_tx_par[i].par[frameIndex].msg.data[0] & 0xf);
						uint32_t checkLen = (uint32_t)(len - txIndex);
						if (checkLen > cf_tx_data_len)
						{
							checkLen = cf_tx_data_len;
						}
						LONGS_EQUAL(0, memcmp(&last_tx_par[i].par[frameIndex].msg.data[1], &payload[txIndex], checkLen));
						frameIndex++;
						txIndex += checkLen;

						poll_time += waitTime - 1;
						iso_can_tp_poll(&link[i], poll_time);
						LONGS_EQUAL(1 + k, last_tx_par[i].cnt);
						poll_time += 1;
						iso_can_tp_poll(&link[i], poll_time);
					}


					LONGS_EQUAL(1, last_tx_done_par[i].cnt);
					LONGS_EQUAL(len, last_tx_done_par[i].par[0].size);
					LONGS_EQUAL(N_OK, last_tx_done_par[i].par[0].error);
					LONGS_EQUAL(0, memcmp(last_tx_done_par[i].par[0].payload, &payload[0], (size_t)len));
				}
				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

//----------------��֡�쳣���ʹ���-----------------------
//�������س�ʱ
//ff��cf���ͳ�ʱ

//----------------��֡�������մ���-----------------------
/*��֤ģ���������������չ��̣���ʱ���ջ��幻�󣬲���Ҫ����*/
//����������
//1. ���Թ�������L_Data_request�����ڵ���iso_can_tp_L_Data_confirm
//���Բ��裺
//1. �򱻲�ͨ��������֡��������Ϣ��8-4095
//2. ����ͨ���ظ����أ��ò���ģ�龡���ܷ��ͣ�BS=0  STMIN=0��
//3. ����ʣ�౨�ģ�����Ƿ���ȷ����
//�ڴ������
//1. ���Դ�����ȷ�յ���֡
int normalRx_tx_CAN_frame_with_done(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	iso_can_tp_L_Data_confirm(link, msg, 0);
	return 0;
}
TEST(multiTest, normalRx)
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
				if (len > 0xfff) len_inc = sizeof(payload) / 20;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------

				L_Data_request_hook = normalRx_tx_CAN_frame_with_done;
				struct CAN_msg FFMsg;
				FFMsg.dlc = TX_DLC;
				FFMsg.id = init[i].rx_id;
				if (len > 0xfff)
				{
					FFMsg.data[0] = 0x10;
					FFMsg.data[1] = 0;
					FFMsg.data[2] = (uint8_t)(len>>24);
					FFMsg.data[3] = (uint8_t)(len >> 16);
					FFMsg.data[4] = (uint8_t)(len >> 8);
					FFMsg.data[5] = (uint8_t)(len);
					memcpy(&FFMsg.data[6], &payload[0], dlc2len(TX_DLC) - 6);
				}
				else {
					FFMsg.data[0] = (uint8_t)(0x10 | (len >> 8));
					FFMsg.data[1] = (uint8_t)len;
					memcpy(&FFMsg.data[2], &payload[0], dlc2len(TX_DLC) - 2);
				}
				iso_can_tp_L_Data_indication(&link[i], &FFMsg);

				LONGS_EQUAL(1, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[0].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[0].msg.id.isExt);
				LONGS_EQUAL(TX_DLC, last_tx_par[i].par[0].msg.dlc);
				LONGS_EQUAL(0x30, last_tx_par[i].par[0].msg.data[0]);
				LONGS_EQUAL(0x0, last_tx_par[i].par[0].msg.data[1]);
				LONGS_EQUAL(0x0, last_tx_par[i].par[0].msg.data[2]);
				for (int k = 3; k < dlc2len(TX_DLC); k++)
				{
					LONGS_EQUAL(init[i].frame_pad, last_tx_par[i].par[0].msg.data[k]);
				}


				uint32_t txFrameNum;
				uint64_t txIndex;
				uint16_t cf_tx_data_len = (dlc2len(TX_DLC) - 1);
				{
					uint16_t ff_tx_data_len;
					if (len <= 0xfff) {
						ff_tx_data_len = dlc2len(TX_DLC) - 2;
					}
					else {
						ff_tx_data_len = dlc2len(TX_DLC) - 6;
					}
					txIndex = ff_tx_data_len;
					txFrameNum = 1 + (int)((len - ff_tx_data_len) / cf_tx_data_len) + (int)!!((len - ff_tx_data_len) % cf_tx_data_len);
				}
				uint8_t SN = 0;
				uint32_t frameIndex = 1;

				for (; frameIndex < txFrameNum; frameIndex++)
				{
					struct CAN_msg CFMsg;
					uint32_t tx_len = (uint32_t)(len - txIndex);
					if (tx_len > cf_tx_data_len)
					{
						tx_len = cf_tx_data_len;
					}
					SN++;
					CFMsg.dlc = TX_DLC;
					CFMsg.id = init[i].rx_id;
					CFMsg.data[0] = 0x20 | (SN & 0xf);
					memcpy(&CFMsg.data[1], &payload[txIndex], tx_len);
					txIndex += tx_len;
					iso_can_tp_L_Data_indication(&link[i], &CFMsg);

					LONGS_EQUAL(1, last_tx_par[i].cnt);
				}
				LONGS_EQUAL(1, last_rx_par[i].cnt);
				LONGS_EQUAL(len, last_rx_par[i].par[0].size);
				LONGS_EQUAL(0, memcmp(last_rx_par[i].par[0].payload, &payload[0], (size_t)len));

				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

/*��֤ģ���������������չ��̣���ʱ���ջ��幻����Ҫ����BS*/
//����������
//1. ���Թ�������L_Data_request�����ڵ���iso_can_tp_L_Data_confirm
//���Բ��裺
//1. �򱻲�ͨ��������֡��������Ϣ��4095
//2. ����ͨ���ظ����أ��ò���ģ�龡���ܷ��ͣ�BS=1-255  STMIN=0��
//3. ����BSֵ����ʣ�౨�ģ�����Ƿ���ȷ����
//�ڴ������
//1. ���Դ�����ȷ�յ���֡
int normalRxBS_tx_CAN_frame_with_done(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	iso_can_tp_L_Data_confirm(link, msg, 0);
	return 0;
}
TEST(multiTest, normalRxBS)
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
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff / 5 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 8;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------

				for (uint16_t BS = 1; BS < 256; BS += 1)
				{
					testPrintf("test for bs %d.\n", BS);
					init_all_modules();
					init[i].FC_BS = (uint8_t)BS;
					LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));

					L_Data_request_hook = normalRx_tx_CAN_frame_with_done;
					init_last_tx_par_vars();
					init_last_tx_done_vars();
					init_last_tp_recieve_vars();

					struct CAN_msg FFMsg;
					FFMsg.dlc = TX_DLC;
					FFMsg.id = init[i].rx_id;
					if (len > 0xfff)
					{
						FFMsg.data[0] = 0x10;
						FFMsg.data[1] = 0;
						FFMsg.data[2] = (uint8_t)(len >> 24);
						FFMsg.data[3] = (uint8_t)(len >> 16);
						FFMsg.data[4] = (uint8_t)(len >> 8);
						FFMsg.data[5] = (uint8_t)(len);
						memcpy(&FFMsg.data[6], &payload[0], dlc2len(TX_DLC) - 6);
					}
					else {
						FFMsg.data[0] = (uint8_t)(0x10 | (len >> 8));
						FFMsg.data[1] = (uint8_t)len;
						memcpy(&FFMsg.data[2], &payload[0], dlc2len(TX_DLC) - 2);
					}
					iso_can_tp_L_Data_indication(&link[i], &FFMsg);

					uint32_t txFrameNum;
					uint64_t txIndex;
					uint16_t cf_tx_data_len = (dlc2len(TX_DLC) - 1);
					{
						uint16_t ff_tx_data_len;
						if (len <= 0xfff) {
							ff_tx_data_len = dlc2len(TX_DLC) - 2;
						}
						else {
							ff_tx_data_len = dlc2len(TX_DLC) - 6;
						}
						txIndex = ff_tx_data_len;
						txFrameNum = 1 + (int)((len - ff_tx_data_len) / cf_tx_data_len) + (int)!!((len - ff_tx_data_len) % cf_tx_data_len);
					}
					uint8_t SN = 0;
					uint32_t frameIndex = 1;

					uint32_t exp_rx_fc_num = 0;
					for (frameIndex = 1; frameIndex < txFrameNum; frameIndex++)
					{
						//testPrintf("test for frameIndex %d.\n", frameIndex);
						if ((frameIndex - 1) % BS == 0)
						{
							exp_rx_fc_num++;
							LONGS_EQUAL(exp_rx_fc_num, last_tx_par[i].cnt);
							LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.id);
							LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.isExt);
							LONGS_EQUAL(0x30, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[0]);
							LONGS_EQUAL(BS, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[1]);
							LONGS_EQUAL(0x0, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[2]);
						}
						struct CAN_msg CFMsg;
						int tx_len = (int)(len - txIndex);
						if (tx_len > cf_tx_data_len) tx_len = cf_tx_data_len;
						SN++;
						CFMsg.dlc = TX_DLC;
						CFMsg.id = init[i].rx_id;
						CFMsg.data[0] = 0x20 | (SN & 0xf);
						memcpy(&CFMsg.data[1], &payload[txIndex], tx_len);
						txIndex += tx_len;
						iso_can_tp_L_Data_indication(&link[i], &CFMsg);
					}
					//no more fc frame after tx done
					LONGS_EQUAL(exp_rx_fc_num, last_tx_par[i].cnt);

					LONGS_EQUAL(1, last_rx_par[i].cnt);
					LONGS_EQUAL(len, last_rx_par[i].par[0].size);
					LONGS_EQUAL(0, memcmp(last_rx_par[i].par[0].payload, &payload[0], (size_t)len));

					L_Data_request_hook = NULL;
				}
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}
/*��֤ģ�鷢�������ر�����û�л�÷������ȷ��ʱ������ִ��֮��Ľ��ղ��� - ��bug��songarpore@hotmail.com�ṩ */
//����������
//���Բ��裺
//1. �򱻲�ͨ��������֡
//2. ����ͨ���ظ�����
//3. �޸����ر��ķ���ȷ�ϱ�������
//�ڴ������
//1. ���Դ��벻Ӧ�ü�������ʣ���֡����
int wrongFCConfirm_tx_CAN_frame_with_done(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	struct CAN_msg msgTmp = *msg;
	if (msgTmp.data[0] == 0x30)
	{
		msgTmp.data[dlc2len(msgTmp.dlc) - 1]++;
	}
	else {
	}
	iso_can_tp_L_Data_confirm(link, &msgTmp, 0);
	return 0;
}
TEST(multiTest, wrongFCConfirm)
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
			uint64_t len_inc = 1000;
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff / 5 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 2;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------

				uint16_t BS = 0;
				{
					//testPrintf("test for bs %d.\n", BS);
					init_all_modules();
					init[i].FC_BS = (uint8_t)BS;
					LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));

					L_Data_request_hook = wrongFCConfirm_tx_CAN_frame_with_done;
					init_last_tx_par_vars();
					init_last_tx_done_vars();
					init_last_tp_recieve_vars();

					struct CAN_msg FFMsg;
					FFMsg.dlc = TX_DLC;
					FFMsg.id = init[i].rx_id;
					if (len > 0xfff)
					{
						FFMsg.data[0] = 0x10;
						FFMsg.data[1] = 0;
						FFMsg.data[2] = (uint8_t)(len >> 24);
						FFMsg.data[3] = (uint8_t)(len >> 16);
						FFMsg.data[4] = (uint8_t)(len >> 8);
						FFMsg.data[5] = (uint8_t)(len);
						memcpy(&FFMsg.data[6], &payload[0], dlc2len(TX_DLC) - 6);
					}
					else {
						FFMsg.data[0] = (uint8_t)(0x10 | (len >> 8));
						FFMsg.data[1] = (uint8_t)len;
						memcpy(&FFMsg.data[2], &payload[0], dlc2len(TX_DLC) - 2);
					}
					iso_can_tp_L_Data_indication(&link[i], &FFMsg);

					uint32_t txFrameNum;
					uint64_t txIndex;
					uint16_t cf_tx_data_len = (dlc2len(TX_DLC) - 1);
					{
						uint16_t ff_tx_data_len;
						if (len <= 0xfff) {
							ff_tx_data_len = dlc2len(TX_DLC) - 2;
						}
						else {
							ff_tx_data_len = dlc2len(TX_DLC) - 6;
						}
						txIndex = ff_tx_data_len;
						txFrameNum = 1 + (int)((len - ff_tx_data_len) / cf_tx_data_len) + (int)!!((len - ff_tx_data_len) % cf_tx_data_len);
					}
					uint8_t SN = 0;
					uint32_t frameIndex = 1;

					uint32_t exp_rx_fc_num = 0;
					for (frameIndex = 1; frameIndex < txFrameNum; frameIndex++)
					{
						//testPrintf("test for frameIndex %d.\n", frameIndex);
						struct CAN_msg CFMsg;
						int tx_len = (int)(len - txIndex);
						if (tx_len > cf_tx_data_len) tx_len = cf_tx_data_len;
						SN++;
						CFMsg.dlc = TX_DLC;
						CFMsg.id = init[i].rx_id;
						CFMsg.data[0] = 0x20 | (SN & 0xf);
						memcpy(&CFMsg.data[1], &payload[txIndex], tx_len);
						txIndex += tx_len;
						iso_can_tp_L_Data_indication(&link[i], &CFMsg);
					}

					LONGS_EQUAL(0, last_rx_par[i].cnt);

					L_Data_request_hook = NULL;
				}
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

/*��֤ģ���������������չ��̣���ʱ���ջ��岻���󣬾ܾ�����*/
//����������
//1. ���Թ�������L_Data_request�����ڵ���iso_can_tp_L_Data_confirm
//���Բ��裺
//1. ��ʼ������ͨ��������ջ��峤��Ϊ8-4094
//2. �򱻲�ģ�鷢����֡������Ϊ���峤��+1
//3. �鿴���ػظ�
//�ڴ������
//1. ���Դ���ܾ�����
TEST(multiTest, normalRxBuffOverrun)
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
			for (uint64_t len = sizeof(payload)+1; len <= 0xffffffff; len += (0xffffffff - sizeof(payload))/5000 + 1)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 15;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------
				L_Data_request_hook = normalRx_tx_CAN_frame_with_done;
				struct CAN_msg FFMsg;
				FFMsg.dlc = TX_DLC;
				FFMsg.id = init[i].rx_id;
				if (len > 0xfff)
				{
					FFMsg.data[0] = 0x10;
					FFMsg.data[1] = 0;
					FFMsg.data[2] = (uint8_t)(len >> 24);
					FFMsg.data[3] = (uint8_t)(len >> 16);
					FFMsg.data[4] = (uint8_t)(len >> 8);
					FFMsg.data[5] = (uint8_t)(len);
					memcpy(&FFMsg.data[6], &payload[0], dlc2len(TX_DLC) - 6);
				}
				else {
					FFMsg.data[0] = (uint8_t)(0x10 | (len >> 8));
					FFMsg.data[1] = (uint8_t)len;
					memcpy(&FFMsg.data[2], &payload[0], dlc2len(TX_DLC) - 2);
				}
				iso_can_tp_L_Data_indication(&link[i], &FFMsg);

				LONGS_EQUAL(1, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[0].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[0].msg.id.isExt);
				LONGS_EQUAL(0x32, last_tx_par[i].par[0].msg.data[0]);

				LONGS_EQUAL(0, last_rx_par[i].cnt);

				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

//----------------�쳣���մ���-----------------------

/*��֤ģ����Դ����֡���չ������յ��ĵ�֡*/
//����������
//���Բ��裺
//1. ��ʼ������ͨ��������ջ��峤��Ϊ4095����������Ϊ4095�Ķ�֡����
//2. �ڵ�10֡CF���ķ���ʱȡ�����Ͳ���һ���µĵ�֡
//�ڴ������
//1. ��2�������ϲ�㱨 N_UNEXP_PDU����
//2. ����㱨��������㱨��֡��Ϣ
TEST(multiTest, unexpectSF)
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
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff / 2 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 2;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------

				L_Data_request_hook = normalRx_tx_CAN_frame_with_done;
				int poll_time = 0;

				iso_can_tp_poll(&link[i], poll_time);
				struct CAN_msg FFMsg;
				FFMsg.dlc = TX_DLC;
				FFMsg.id = init[i].rx_id;
				if (len > 0xfff)
				{
					FFMsg.data[0] = 0x10;
					FFMsg.data[1] = 0;
					FFMsg.data[2] = (uint8_t)(len >> 24);
					FFMsg.data[3] = (uint8_t)(len >> 16);
					FFMsg.data[4] = (uint8_t)(len >> 8);
					FFMsg.data[5] = (uint8_t)(len);
					memcpy(&FFMsg.data[6], &payload[0], dlc2len(TX_DLC) - 6);
				}
				else {
					FFMsg.data[0] = (uint8_t)(0x10 | (len >> 8));
					FFMsg.data[1] = (uint8_t)len;
					memcpy(&FFMsg.data[2], &payload[0], dlc2len(TX_DLC) - 2);
				}
				iso_can_tp_L_Data_indication(&link[i], &FFMsg);

				uint16_t exp_rx_fc_num = 1;
				int BS = 0;
				LONGS_EQUAL(exp_rx_fc_num, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.isExt);
				LONGS_EQUAL(TX_DLC, last_tx_par[i].par[exp_rx_fc_num - 1].msg.dlc);
				LONGS_EQUAL(0x30, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[0]);
				LONGS_EQUAL(BS, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[1]);
				LONGS_EQUAL(0x0, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[2]);

				uint32_t txFrameNum;
				uint64_t txIndex;
				uint16_t cf_tx_data_len = (dlc2len(TX_DLC) - 1);
				{
					uint16_t ff_tx_data_len;
					if (len <= 0xfff) {
						ff_tx_data_len = dlc2len(TX_DLC) - 2;
					}
					else {
						ff_tx_data_len = dlc2len(TX_DLC) - 6;
					}
					txIndex = ff_tx_data_len;
					txFrameNum = 1 + (int)((len - ff_tx_data_len) / cf_tx_data_len) + (int)!!((len - ff_tx_data_len) % cf_tx_data_len);
				}
				uint8_t SN = 0;
				uint32_t frameIndex = 1;

				for (frameIndex = 1; (frameIndex + 10) < txFrameNum; frameIndex++)
				{
					struct CAN_msg CFMsg;
					int tx_len = (int)(len - txIndex);
					if (tx_len > cf_tx_data_len) tx_len = cf_tx_data_len;
					SN++;
					CFMsg.dlc = TX_DLC;
					CFMsg.id = init[i].rx_id;
					CFMsg.data[0] = 0x20 | (SN & 0xf);
					memcpy(&CFMsg.data[1], &payload[txIndex], tx_len);
					txIndex += tx_len;
					iso_can_tp_L_Data_indication(&link[i], &CFMsg);
				}


				struct CAN_msg SFMsg;
				SFMsg.dlc = 8;
				SFMsg.id = init[i].rx_id;
				SFMsg.data[0] = 0x04;
				SFMsg.data[1] = 3;
				SFMsg.data[2] = 3;
				SFMsg.data[3] = 2;
				SFMsg.data[4] = 1;
				memcpy(&SFMsg.data[2], &payload[0], 6);
				iso_can_tp_L_Data_indication(&link[i], &SFMsg);


				LONGS_EQUAL(2, last_rx_par[i].cnt);
				LONGS_EQUAL(len, last_rx_par[i].par[0].size);
				LONGS_EQUAL(N_UNEXP_PDU, last_rx_par[i].par[0].error);

				LONGS_EQUAL(SFMsg.data[0], last_rx_par[i].par[1].size);
				LONGS_EQUAL(N_OK, last_rx_par[i].par[1].error);
				LONGS_EQUAL(SFMsg.data[1], last_rx_par[i].par[1].payload[0]);
				LONGS_EQUAL(SFMsg.data[2], last_rx_par[i].par[1].payload[1]);
				LONGS_EQUAL(SFMsg.data[3], last_rx_par[i].par[1].payload[2]);
				LONGS_EQUAL(SFMsg.data[4], last_rx_par[i].par[1].payload[3]);
				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

/*��֤ģ����Դ����֡���չ������յ��Ķ�֡*/
//����������
//���Բ��裺
//1. ��ʼ������ͨ��������ջ��峤��Ϊ4095����������Ϊ4095�Ķ�֡����
//2. �ڵ�10֡CF���ķ���ʱȡ�����Ͳ���һ���µĳ���Ϊ4095�Ķ�֡����
//�ڴ������
//1. ��2�������ϲ�㱨 N_UNEXP_PDU����
//2. ����㱨����������ղ��㱨�µĶ�֡��Ϣ
TEST(multiTest, unexpectFF)
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
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff / 2 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 2;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------

				L_Data_request_hook = normalRx_tx_CAN_frame_with_done;
				int poll_time = 0;

				iso_can_tp_poll(&link[i], poll_time);
				struct CAN_msg FFMsg;
				FFMsg.dlc = TX_DLC;
				FFMsg.id = init[i].rx_id;
				if (len > 0xfff)
				{
					FFMsg.data[0] = 0x10;
					FFMsg.data[1] = 0;
					FFMsg.data[2] = (uint8_t)(len >> 24);
					FFMsg.data[3] = (uint8_t)(len >> 16);
					FFMsg.data[4] = (uint8_t)(len >> 8);
					FFMsg.data[5] = (uint8_t)(len);
					memcpy(&FFMsg.data[6], &payload[0], dlc2len(TX_DLC) - 6);
				}
				else {
					FFMsg.data[0] = (uint8_t)(0x10 | (len >> 8));
					FFMsg.data[1] = (uint8_t)len;
					memcpy(&FFMsg.data[2], &payload[0], dlc2len(TX_DLC) - 2);
				}
				iso_can_tp_L_Data_indication(&link[i], &FFMsg);

				uint16_t exp_rx_fc_num = 1;
				int BS = 0;
				LONGS_EQUAL(exp_rx_fc_num, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.isExt);
				LONGS_EQUAL(0x30, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[0]);
				LONGS_EQUAL(BS, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[1]);
				LONGS_EQUAL(0x0, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[2]);

				uint32_t txFrameNum;
				uint64_t txIndex;
				uint16_t cf_tx_data_len = (dlc2len(TX_DLC) - 1);
				{
					uint16_t ff_tx_data_len;
					if (len <= 0xfff) {
						ff_tx_data_len = dlc2len(TX_DLC) - 2;
					}
					else {
						ff_tx_data_len = dlc2len(TX_DLC) - 6;
					}
					txIndex = ff_tx_data_len;
					txFrameNum = 1 + (int)((len - ff_tx_data_len) / cf_tx_data_len) + (int)!!((len - ff_tx_data_len) % cf_tx_data_len);
				}
				uint8_t SN = 0;
				uint32_t frameIndex = 1;

				for (frameIndex = 1; (frameIndex + 10) < txFrameNum; frameIndex++)
				{
					struct CAN_msg CFMsg;
					int tx_len = (int)(len - txIndex);
					if (tx_len > cf_tx_data_len) tx_len = cf_tx_data_len;
					SN++;
					CFMsg.dlc = TX_DLC;
					CFMsg.id = init[i].rx_id;
					CFMsg.data[0] = 0x20 | (SN & 0xf);
					memcpy(&CFMsg.data[1], &payload[txIndex], tx_len);
					txIndex += tx_len;
					iso_can_tp_L_Data_indication(&link[i], &CFMsg);
				}
				init_last_tx_par_vars();
				init_last_tx_done_vars();
				init_last_tp_recieve_vars();

				iso_can_tp_L_Data_indication(&link[i], &FFMsg);

				LONGS_EQUAL(1, last_rx_par[i].cnt);
				LONGS_EQUAL(len, last_rx_par[i].par[0].size);
				LONGS_EQUAL(N_UNEXP_PDU, last_rx_par[i].par[0].error);
				init_last_tp_recieve_vars();

				exp_rx_fc_num = 1;
				BS = 0;
				LONGS_EQUAL(exp_rx_fc_num, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.isExt);
				LONGS_EQUAL(0x30, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[0]);
				LONGS_EQUAL(BS, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[1]);
				LONGS_EQUAL(0x0, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[2]);

				 cf_tx_data_len = (dlc2len(TX_DLC) - 1);
				{
					uint16_t ff_tx_data_len;
					if (len <= 0xfff) {
						ff_tx_data_len = dlc2len(TX_DLC) - 2;
					}
					else {
						ff_tx_data_len = dlc2len(TX_DLC) - 6;
					}
					txIndex = ff_tx_data_len;
					txFrameNum = 1 + (int)((len - ff_tx_data_len) / cf_tx_data_len) + (int)!!((len - ff_tx_data_len) % cf_tx_data_len);
				}
				SN = 0;
				frameIndex = 1;

				for (frameIndex = 1; frameIndex < txFrameNum; frameIndex++)
				{
					struct CAN_msg CFMsg;
					int tx_len = (int)(len - txIndex);
					if (tx_len > cf_tx_data_len) tx_len = cf_tx_data_len;
					SN++;
					CFMsg.dlc = TX_DLC;
					CFMsg.id = init[i].rx_id;
					CFMsg.data[0] = 0x20 | (SN & 0xf);
					memcpy(&CFMsg.data[1], &payload[txIndex], tx_len);
					txIndex += tx_len;
					iso_can_tp_L_Data_indication(&link[i], &CFMsg);
				}

				LONGS_EQUAL(1, last_rx_par[i].cnt);
				LONGS_EQUAL(len, last_rx_par[i].par[0].size);
				LONGS_EQUAL(0, memcmp(last_rx_par[i].par[0].payload, &payload[0], (size_t)len));
				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}


/*��֤ģ�������������SN���󣬿���������ǰ���ղ����½����µı���*/
//����������
//���Բ��裺
//1. ��ʼ������ͨ��������ջ��峤��Ϊ4095����������Ϊ4095�Ķ�֡����
//2. ����SN��Ŵ���
//�ڴ������
//1. ��2�������ϲ�㱨N_WRONG_SN����
TEST(multiTest, unexpectSN)
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
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff/1 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 2;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------
				int poll_time = 0;

				L_Data_request_hook = normalRx_tx_CAN_frame_with_done;
				iso_can_tp_poll(&link[i], poll_time);

				struct CAN_msg FFMsg;
				FFMsg.dlc = TX_DLC;
				FFMsg.id = init[i].rx_id;
				if (len > 0xfff)
				{
					FFMsg.data[0] = 0x10;
					FFMsg.data[1] = 0;
					FFMsg.data[2] = (uint8_t)(len >> 24);
					FFMsg.data[3] = (uint8_t)(len >> 16);
					FFMsg.data[4] = (uint8_t)(len >> 8);
					FFMsg.data[5] = (uint8_t)(len);
					memcpy(&FFMsg.data[6], &payload[0], dlc2len(TX_DLC) - 6);
				}
				else {
					FFMsg.data[0] = (uint8_t)(0x10 | (len >> 8));
					FFMsg.data[1] = (uint8_t)len;
					memcpy(&FFMsg.data[2], &payload[0], dlc2len(TX_DLC) - 2);
				}
				iso_can_tp_L_Data_indication(&link[i], &FFMsg);

				uint16_t exp_rx_fc_num = 1;
				int BS = 0;
				LONGS_EQUAL(exp_rx_fc_num, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.isExt);
				LONGS_EQUAL(0x30, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[0]);
				LONGS_EQUAL(BS, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[1]);
				LONGS_EQUAL(0x0, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[2]);

				uint32_t txFrameNum;
				uint64_t txIndex;
				uint16_t cf_tx_data_len = (dlc2len(TX_DLC) - 1);
				{
					uint16_t ff_tx_data_len;
					if (len <= 0xfff) {
						ff_tx_data_len = dlc2len(TX_DLC) - 2;
					}
					else {
						ff_tx_data_len = dlc2len(TX_DLC) - 6;
					}
					txIndex = ff_tx_data_len;
					txFrameNum = 1 + (int)((len - ff_tx_data_len) / cf_tx_data_len) + (int)!!((len - ff_tx_data_len) % cf_tx_data_len);
				}
				uint8_t SN = 0;
				uint32_t frameIndex = 1;

				for (frameIndex = 1; frameIndex < txFrameNum; frameIndex++)
				{
					struct CAN_msg CFMsg;
					int tx_len = (int)(len - txIndex);
					if (tx_len > cf_tx_data_len) tx_len = cf_tx_data_len;
					SN++;
					if (frameIndex == txFrameNum / 2)  SN++;
					CFMsg.dlc = TX_DLC;
					CFMsg.id = init[i].rx_id;
					CFMsg.data[0] = 0x20 | (SN & 0xf);
					memcpy(&CFMsg.data[1], &payload[txIndex], tx_len);
					txIndex += tx_len;
					iso_can_tp_L_Data_indication(&link[i], &CFMsg);

					if (frameIndex == txFrameNum / 2) {
						LONGS_EQUAL(1, last_rx_par[i].cnt);
						LONGS_EQUAL(len, last_rx_par[i].par[0].size);
						LONGS_EQUAL(N_WRONG_SN, last_rx_par[i].par[0].error);
						SN--;
					}
				}
				LONGS_EQUAL(1, last_rx_par[i].cnt);
				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

/*��֤ģ����Դ���FC���쳣��FlowStatusֵ*/
//����������
//���Բ��裺
//1. ��ʼ������ͨ��������ջ��峤��Ϊ4095����������Ϊ4095�Ķ�֡����
//2. �ڵ�1֡���ر����з��ͷǷ���FlowStatusֵ
//�ڴ������
//1. ��2�������ϲ�㱨 N_INVALID_FS����
TEST(multiTest, unexpectFlowStatus)
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
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff / 2 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 2;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------

				L_Data_request_hook = txFcWait_tx_CAN_frame_with_done;
				iso_can_tp_poll(&link[i], 0);
				for (uint32_t j = 3; j < 16; j++)
				{
					init_last_tx_par_vars();
					init_last_tx_done_vars();
					LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, (uint32_t)len));

					struct CAN_msg flowControlMsg;
					flowControlMsg.dlc = TX_DLC;
					flowControlMsg.id = init[i].rx_id;
					memset(flowControlMsg.data, 0, flowControlMsg.dlc);
					flowControlMsg.data[0] = 0x30 | j;
					iso_can_tp_L_Data_indication(&link[i], &flowControlMsg);

					iso_can_tp_poll(&link[i], j * 5);
					LONGS_EQUAL(1, last_tx_par[i].cnt);
					LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[0].msg.id.id);
					LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[0].msg.id.isExt);
					LONGS_EQUAL(0x10, last_tx_par[i].par[0].msg.data[0] & 0xf0);

					LONGS_EQUAL(1, last_tx_done_par[i].cnt);
					LONGS_EQUAL(N_INVALID_FS, last_tx_done_par[i].par[0].error);
				}
				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

/*��֤ģ����Դ���FC���쳣��STminֵ*/
//�Ѿ���txStmin��������ɲ���

//----------------���շ���ʱ����-----------------------
int tx_FF_N_AS_Timeout_tx_ok_done;//0:����-1    1: ����0     ����������iso_can_tp_L_Data_confirm�󷵻�0
int tx_FF_N_AS_Timeout_tx_CAN_frame_with_done(can_iso_tp_link_t_p link, const struct CAN_msg* msg)
{
	if (0 == tx_FF_N_AS_Timeout_tx_ok_done)
	{
		return -1;
	}if (1 == tx_FF_N_AS_Timeout_tx_ok_done)
	{
		return 0;
	}
	iso_can_tp_L_Data_confirm(link, msg, 0);
	return 0;
}
/*��֤ģ����Դ������ط��ͳ�ʱN_AR*/
//����������
//���Բ��裺
//1. ��ʼ������ͨ��������ջ��峤��Ϊ4095���򱻲���뷢������Ϊ4095�Ķ�֡����
//2. �ײ�㱨���ط���ʧ��
//3. ��N_ARʱ���ڣ�����POLL���������������ͣ��ײ�㱨���ͳɹ�����confirm
//4. ��N_ARʱ���ڣ�����POLL��������������
//5. ��N_ARʱ���⣬����POLL����
//�ڴ������
//1. ��5����������ϲ�㱨����ʧ���¼���ʧ��ԭ��Ϊ N_TIMEOUT_A
TEST(multiTest, rx_FC_N_AR_Timeout)
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
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff / 2 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 2;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------
				L_Data_request_hook = tx_FF_N_AS_Timeout_tx_CAN_frame_with_done;
				int poll_time = 0;

				iso_can_tp_poll(&link[i], poll_time);

				tx_FF_N_AS_Timeout_tx_ok_done = 0;
				struct CAN_msg FFMsg;
				FFMsg.dlc = TX_DLC;
				FFMsg.id = init[i].rx_id;
				if (len > 0xfff)
				{
					FFMsg.data[0] = 0x10;
					FFMsg.data[1] = 0;
					FFMsg.data[2] = (uint8_t)(len >> 24);
					FFMsg.data[3] = (uint8_t)(len >> 16);
					FFMsg.data[4] = (uint8_t)(len >> 8);
					FFMsg.data[5] = (uint8_t)(len);
					memcpy(&FFMsg.data[6], &payload[0], dlc2len(TX_DLC) - 6);
				}
				else {
					FFMsg.data[0] = (uint8_t)(0x10 | (len >> 8));
					FFMsg.data[1] = (uint8_t)len;
					memcpy(&FFMsg.data[2], &payload[0], dlc2len(TX_DLC) - 2);
				}
				iso_can_tp_L_Data_indication(&link[i], &FFMsg);

				uint16_t exp_rx_fc_num = 1;
				int BS = 0;
				LONGS_EQUAL(exp_rx_fc_num, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.isExt);
				LONGS_EQUAL(0x30, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[0]);
				LONGS_EQUAL(BS, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[1]);
				LONGS_EQUAL(0x0, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[2]);


				tx_FF_N_AS_Timeout_tx_ok_done = 1;
				poll_time += init[i].N_Ar - 2;
				iso_can_tp_poll(&link[i], poll_time);
				exp_rx_fc_num = 2;
				LONGS_EQUAL(exp_rx_fc_num, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.isExt);
				LONGS_EQUAL(0x30, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[0]);
				LONGS_EQUAL(BS, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[1]);
				LONGS_EQUAL(0x0, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[2]);

				tx_FF_N_AS_Timeout_tx_ok_done = 2;
				poll_time += init[i].N_Ar - 1;
				iso_can_tp_poll(&link[i], poll_time);
				exp_rx_fc_num = 2;
				LONGS_EQUAL(exp_rx_fc_num, last_tx_par[i].cnt);

				poll_time += init[i].N_Ar + 1;
				iso_can_tp_poll(&link[i], poll_time);

				LONGS_EQUAL(1, last_rx_par[i].cnt);
				LONGS_EQUAL(len, last_rx_par[i].par[0].size);
				LONGS_EQUAL(N_TIMEOUT_A, last_rx_par[i].par[0].error);
				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

/*��֤ģ����Դ���CF���ճ�ʱN_CR*/
//����������
//���Բ��裺
//1. ��ʼ������ͨ��������ջ��峤��Ϊ4095����������Ϊ4095�Ķ�֡����
//2. �յ�����������ر��Ĳ�ȷ�Ϸ������
//3. ��N_CRʱ���ڣ�����POLL����
//4. ��N_CRʱ���⣬����POLL����
//�ڴ������
//1. ��4����������ϲ�㱨����ʧ���¼���ʧ��ԭ��Ϊ N_TIMEOUT_Cr
TEST(multiTest, rx_CF_N_CR_Timeout)
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
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff / 2 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 2;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------
				L_Data_request_hook = tx_FF_N_AS_Timeout_tx_CAN_frame_with_done;
				int poll_time = 0;

				iso_can_tp_poll(&link[i], poll_time);

				tx_FF_N_AS_Timeout_tx_ok_done = 2;
				struct CAN_msg FFMsg;
				FFMsg.dlc = TX_DLC;
				FFMsg.id = init[i].rx_id;
				if (len > 0xfff)
				{
					FFMsg.data[0] = 0x10;
					FFMsg.data[1] = 0;
					FFMsg.data[2] = (uint8_t)(len >> 24);
					FFMsg.data[3] = (uint8_t)(len >> 16);
					FFMsg.data[4] = (uint8_t)(len >> 8);
					FFMsg.data[5] = (uint8_t)(len);
					memcpy(&FFMsg.data[6], &payload[0], dlc2len(TX_DLC) - 6);
				}
				else {
					FFMsg.data[0] = (uint8_t)(0x10 | (len >> 8));
					FFMsg.data[1] = (uint8_t)len;
					memcpy(&FFMsg.data[2], &payload[0], dlc2len(TX_DLC) - 2);
				}
				iso_can_tp_L_Data_indication(&link[i], &FFMsg);

				uint16_t exp_rx_fc_num = 1;
				int BS = 0;
				LONGS_EQUAL(exp_rx_fc_num, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[exp_rx_fc_num - 1].msg.id.isExt);
				LONGS_EQUAL(0x30, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[0]);
				LONGS_EQUAL(BS, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[1]);
				LONGS_EQUAL(0x0, last_tx_par[i].par[exp_rx_fc_num - 1].msg.data[2]);


				tx_FF_N_AS_Timeout_tx_ok_done = 1;
				poll_time += init[i].N_Cr - 1;
				iso_can_tp_poll(&link[i], poll_time);
				LONGS_EQUAL(0, last_rx_par[i].cnt);

				tx_FF_N_AS_Timeout_tx_ok_done = 1;
				poll_time += init[i].N_Cr + 1;
				iso_can_tp_poll(&link[i], poll_time);

				LONGS_EQUAL(1, last_rx_par[i].cnt);
				LONGS_EQUAL(len, last_rx_par[i].par[0].size);
				LONGS_EQUAL(N_TIMEOUT_CR, last_rx_par[i].par[0].error);
			L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

//----------------���ͷ���ʱ����-----------------------

/*��֤ģ����Դ���FF���ͳ�ʱN_AS*/
//����������
//���Բ��裺
//1. ��ʼ������ͨ��������ջ��峤��Ϊ4095�����󱻲���뷢������Ϊ4095�Ķ�֡����
//2. ������뷢��FF���ײ�㱨���ķ���ʧ��
//3. ��N_ASʱ���ڣ�����POLL���������������ͣ��ײ�㱨���ͳɹ�
//4. ��N_ASʱ���ڣ�����POLL��������������
//5. ��N_ASʱ���⣬����POLL����
//�ڴ������
//1. ��5����������ϲ�㱨����ʧ���¼���ʧ��ԭ��Ϊ N_TIMEOUT_A
TEST(multiTest, tx_FF_N_AS_Timeout)
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
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff / 2 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 2;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------

				L_Data_request_hook = tx_FF_N_AS_Timeout_tx_CAN_frame_with_done;
				if (init[i].N_Ar <= 2)
					continue;

				LONGS_EQUAL(OP_OK, iso_can_tp_create(&link[i], &init[i]));
				tx_FF_N_AS_Timeout_tx_ok_done = 0;
				iso_can_tp_poll(&link[i], 0);
				LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, (uint32_t)len));
				LONGS_EQUAL(1, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[0].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[0].msg.id.isExt);
				LONGS_EQUAL(TX_DLC, last_tx_par[i].par[0].msg.dlc);
				LONGS_EQUAL(0x10, last_tx_par[i].par[0].msg.data[0] & 0xf0);

				tx_FF_N_AS_Timeout_tx_ok_done = 1;
				iso_can_tp_poll(&link[i], 1);
				LONGS_EQUAL(2, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[1].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[1].msg.id.isExt);
				LONGS_EQUAL(TX_DLC, last_tx_par[i].par[1].msg.dlc);
				LONGS_EQUAL(0x10, last_tx_par[i].par[1].msg.data[0] & 0xf0);
				LONGS_EQUAL(0, last_tx_done_par[i].cnt);

				//��δ��ʱ  ��������������
				iso_can_tp_poll(&link[i], 2);
				LONGS_EQUAL(2, last_tx_par[i].cnt);
				LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, sizeof(payload)));
				LONGS_EQUAL(2, last_tx_par[i].cnt);
				LONGS_EQUAL(1, last_tx_done_par[i].cnt);

				iso_can_tp_poll(&link[i], init[i].N_Ar + 1);
				LONGS_EQUAL(2, last_tx_done_par[i].cnt);
				LONGS_EQUAL(N_TIMEOUT_A, last_tx_done_par[i].par[1].error);
				//��ʱ���������������
				LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, sizeof(payload)));
				LONGS_EQUAL(3, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[2].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[2].msg.id.isExt);
				LONGS_EQUAL(TX_DLC, last_tx_par[i].par[2].msg.dlc);
				LONGS_EQUAL(0x10, last_tx_par[i].par[2].msg.data[0] & 0xf0);
				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

/*��֤ģ����Դ������ؽ��ճ�ʱN_BS*/
//����������
//���Բ��裺
//1. ��ʼ������ͨ��������ջ��峤��Ϊ4095�����󱻲���뷢������Ϊ4095�Ķ�֡����
//2. ������뷢��FF���������ͳɹ�
//3. ��N_BSʱ���ڣ�����POLL����
//4. ��N_BSʱ���⣬����POLL����
//�ڴ������
//1. ��4����������ϲ�㱨����ʧ���¼���ʧ��ԭ��Ϊ N_TIMEOUT_Bs
TEST(multiTest, tx_FC_N_BS_Timeout)
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
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff / 2 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 2;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------

				L_Data_request_hook = tx_FF_N_AS_Timeout_tx_CAN_frame_with_done;
				if (init[i].N_As <= 2)
					continue;

				tx_FF_N_AS_Timeout_tx_ok_done = 2;
				iso_can_tp_poll(&link[i], 0);
				LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, (uint32_t)len));
				LONGS_EQUAL(1, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[0].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[0].msg.id.isExt);
				LONGS_EQUAL(0x10, last_tx_par[i].par[0].msg.data[0] & 0xf0);

				iso_can_tp_poll(&link[i], init[i].N_As - 1);
				LONGS_EQUAL(0, last_tx_done_par[i].cnt);

				iso_can_tp_poll(&link[i], init[i].N_As + 1);
				LONGS_EQUAL(1, last_tx_done_par[i].cnt);
				LONGS_EQUAL(N_TIMEOUT_BS, last_tx_done_par[i].par[0].error);
				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}

/*��֤ģ����Դ���CF���ͳ�ʱN_AS*/
//����������
//���Բ��裺
//1. ��ʼ������ͨ��������ջ��峤��Ϊ4095�����󱻲���뷢������Ϊ4095�Ķ�֡����
//2. ������뷢��FF���ײ�㱨���ķ��ͳɹ�,�ظ����أ�������뷢��CF
//3. ��N_ASʱ���ڣ�����POLL���������������ͣ��ײ�㱨���ͳɹ�
//4. ��N_ASʱ���ڣ�����POLL��������������
//5. ��N_ASʱ���⣬����POLL����
//�ڴ������
//1. ��5����������ϲ�㱨����ʧ���¼���ʧ��ԭ��Ϊ N_TIMEOUT_A
TEST(multiTest, tx_CF_N_AS_Timeout)
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
			for (uint64_t len = len_start; len < sizeof(payload); len += 0xfff / 2 + len_inc)
			{
				if (len > 0xfff) len_inc = sizeof(payload) / 2;
				init_last_tx_par_vars();
				init_last_tp_recieve_vars();
				init_last_tx_done_vars();
				testPrintf("test for len %d.\n", len);
				//-----------------------------------------------------------------

				L_Data_request_hook = tx_FF_N_AS_Timeout_tx_CAN_frame_with_done;
				if (init[i].N_Ar <= 2)
					continue;

				tx_FF_N_AS_Timeout_tx_ok_done = 2;
				iso_can_tp_poll(&link[i], 0);
				LONGS_EQUAL(0, iso_can_tp_N_USData_request(&link[i], 0, payload, (uint32_t)len));
				LONGS_EQUAL(1, last_tx_par[i].cnt);
				LONGS_EQUAL(init[i].tx_id.id, last_tx_par[i].par[0].msg.id.id);
				LONGS_EQUAL(init[i].tx_id.isExt, last_tx_par[i].par[0].msg.id.isExt);
				LONGS_EQUAL(0x10, last_tx_par[i].par[0].msg.data[0] & 0xf0);


				tx_FF_N_AS_Timeout_tx_ok_done = 0;
				struct CAN_msg flowControlMsg;
				flowControlMsg.dlc = 8;
				flowControlMsg.id = init[i].rx_id;
				memset(flowControlMsg.data, 0, flowControlMsg.dlc);
				flowControlMsg.data[0] = 0x30;
				iso_can_tp_L_Data_indication(&link[i], &flowControlMsg);

				LONGS_EQUAL(2, last_tx_par[i].cnt);
				LONGS_EQUAL(0x21, last_tx_par[i].par[1].msg.data[0]);

				tx_FF_N_AS_Timeout_tx_ok_done = 1;
				iso_can_tp_poll(&link[i], init[i].N_As - 2);
				LONGS_EQUAL(3, last_tx_par[i].cnt);
				LONGS_EQUAL(0x21, last_tx_par[i].par[2].msg.data[0]);

				iso_can_tp_poll(&link[i], init[i].N_As - 1);
				LONGS_EQUAL(3, last_tx_par[i].cnt);

				iso_can_tp_poll(&link[i], init[i].N_As + 1);
				LONGS_EQUAL(3, last_tx_par[i].cnt);

				LONGS_EQUAL(1, last_tx_done_par[i].cnt);
				LONGS_EQUAL(N_TIMEOUT_A, last_tx_done_par[i].par[0].error);
				L_Data_request_hook = NULL;
			}
		}
	}
	printf("time for case %s is %lldms\n", __FUNCTION__, GetTickCount64() - tick_start);
}
