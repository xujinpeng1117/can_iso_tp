#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "testPrint.h"


#include "can_iso_tp.h"
#include "can_iso_tp_private.h"

TEST_GROUP(BaseTest)
{
	uint8_t rx_buff[4095];
    void setup()
    {
		print_enable = false;
    }

    void teardown()
    {
    }
};
//内存使用
TEST(BaseTest, ramUsage)
{
	struct can_iso_tp_link_t link0;
	printf("\n");
	printf("ram used for one channels: %d bytes.\n", sizeof(struct can_iso_tp_link_t) );
	printf("ram used for struct can_iso_tp_link_t: %d bytes.\n", sizeof(struct can_iso_tp_link_t));
	printf("ram used for struct can_iso_tp_init_t: %d bytes.\n", sizeof(struct can_iso_tp_init_t));
	printf("ram used for struct link.tx_record: %d bytes.\n", sizeof(link0.tx_record));
	printf("ram used for struct link.rx_record: %d bytes.\n", sizeof(link0.rx_record));
	printf("ram used for struct link.tx_event: %d bytes.\n", sizeof(link0.tx_events));
	printf("ram used for struct link.rx_event: %d bytes.\n", sizeof(link0.rx_events));
	printf("ram used for struct CAN_msg: %d bytes.\n", sizeof(struct CAN_msg));
	printf("ram used for struct CAN_msg_id: %d bytes.\n", sizeof(struct CAN_msg_id));
	printf("ram used for  e_tx_status: %d bytes.\n", sizeof(e_tx_status));
	printf("\n");
}

//初始化不正确时不能创建tp通道
TEST(BaseTest, id_create_test)
{
	struct can_iso_tp_link_t link0;
	struct can_iso_tp_init_t init0;
	CHECK(iso_can_tp_create(&link0, 0) != OP_OK);

	memset(&init0, 0, sizeof(init0));

	CHECK(iso_can_tp_create(&link0, &init0) != OP_OK);

	init0.rx_id.isExt = 0;
	init0.rx_id.id = 123;
	CHECK(iso_can_tp_create(&link0, &init0) != OP_OK);

	init0.funtion_id.isExt = 0;
	init0.funtion_id.id = 123;
	CHECK(iso_can_tp_create(&link0, &init0) != OP_OK);

}
//初始化多个通道
TEST(BaseTest, multi_create_test)
{
	struct can_iso_tp_link_t link0;
	struct can_iso_tp_link_t link1;
	struct can_iso_tp_init_t init0;
	memset(&init0, 0, sizeof(init0));
	init0.rx_id.id = 0x700;
	init0.tx_id.id = 0x701;
	init0.funtion_id.id = 0x7ff;
	init0.rx_buff = rx_buff;
	init0.rx_buff_len = sizeof(rx_buff);

	LONGS_EQUAL(OP_OK, iso_can_tp_create(&link0, &init0));
	LONGS_EQUAL(OP_OK, iso_can_tp_create(&link1, &init0));
}

//没有tp通道时调用其他函数，运行无效果,也不会崩溃
TEST(BaseTest, none_create_poll_test)
{
	struct can_iso_tp_link_t link0;
	
	uint8_t payload3[] = { 1,2,3 };
	struct CAN_msg msg;

	iso_can_tp_poll(&link0, 0);
	iso_can_tp_L_Data_indication(&link0, 0);
	iso_can_tp_L_Data_indication(&link0, &msg);
	CHECK(OP_OK !=iso_can_tp_N_USData_request(&link0, 0, 0, 0));
	CHECK(OP_OK != iso_can_tp_N_USData_request(&link0, 0, 0, 10));
	CHECK(OP_OK != iso_can_tp_N_USData_request(&link0, 0, payload3, 3));
	iso_can_tp_L_Data_confirm(&link0, 0, 0);
}

