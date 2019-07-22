##CAN_ISO_TP

The purpose of this project is to create a general CAN bus transport layer code, which is written with reference to ISO 15765-2 2016. It has the following characteristics:

* Support 1-0xfffffff byte data transfer (not limited to 4095 bytes)
* Supporting different CAN DLC lengths (8-0x0f), that is, CAN-FD is supported
* Supporting multiple fault handling mechanisms
* Independent of specific platform, support compiling and running on embedded and PC side
* Supporting multiple channels to run without interference when RAM resources are sufficient

Compared with standard protocols, the code has been simplified to a certain extent:

* Mixed address mode is not supported
* N_USData_FF. indication service primitive is not supported

##Use examples

Referring to the test project under example directory, this project uses VS2019 to compile and run (select DEBUG-X86 mode to run), and different parameters will be used to initiate the transmission request at run time. The frames in each transmission request will be recorded in the corresponding text file.

##Resource usage instructions

When CAN_ISO_TP runs, each channel occupies the following resources (counted in VS2019, excluding the receiving and sending buffer occupancy):

* RAM usage in support of CANFD (macro definition SUPPORT_CAN_FD in can_iso_tp.h without comment): 540 bytes
* RAM usage when CANFD (macro definition SUPPORT_CAN_FD in annotation can_iso_tp.h) is not supported: 316 bytes

##Code migration instructions

The code is written in standard C language and can run on multiple platforms. When running on different platforms, attention should be paid to the mcu_lock.h file. MCU_LOCK_ENTER_CRITICAL and MCU_LOCK_EXIT_CRITICAL need to be modified according to the actual hardware. The operation principle is similar to that of code critical zone protection in UCOS-II. If it is only used for simple experimental verification, the default file can be used directly.

##Question Feedback

If you have any questions in use, please give me feedback. You can communicate with me in the following ways

* Email(439590310@qq.com)






##CAN_ISO_TP说明

本项目的目的是创建一个通用的CAN总线传输层代码，代码参考ISO 15765-2 2016版编写，具有以下特点：

* 支持1-0xfffffff字节的数据传输（不仅限于4095字节）
* 支持不同的CAN DLC长度（8-0x0f），即支持CAN-FD
* 支持多种故障处理机制
* 独立于特定平台，支持在嵌入式和PC端编译运行
* 在RAM资源足够的情况下，支持多个通道互不干扰运行

和标准协议相比，代码作出了一定程度的简化：

* 不支持混合地址寻址模式
* 不支持N_USData_FF.indication服务原语

##使用示例

参考example目录下的测试工程，该工程使用VS2019进行编译运行（选择DEBUG-X86模式运行），运行时会使用不同的参数发起传输请求，每一次传输请求过程中的报文均会记录到对应的文本文件中。

##资源使用说明

当CAN_ISO_TP运行时，每个通道占用资源如下（于VS2019中统计，不包含接收和发送缓冲区占用量）：

* 支持CANFD(不注释can_iso_tp.h中宏定义SUPPORT_CAN_FD)时RAM用量：540字节
* 不支持CANFD(注释can_iso_tp.h中宏定义SUPPORT_CAN_FD)时RAM用量：316字节

##代码移植说明

代码采用标准C语言编写，可在多平台运行，在不同平台上运行时需要注意mcu_lock.h文件，其中MCU_LOCK_ENTER_CRITICAL和MCU_LOCK_EXIT_CRITICAL需要按照实际硬件修改，具体其运行原理与UCOS-II中代码临界区保护类似。如果只是简单实验验证使用，直接使用默认文件即可。

##有问题反馈

在使用中有任何问题，欢迎反馈给我，可以用以下联系方式跟我交流

* 邮件(439590310@qq.com)