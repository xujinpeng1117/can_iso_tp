##unit_test

The vs2019 project in this directory (running in debug-x86 mode) is used for unit test of can ISO TP code. The test method refers to the book <<Test-Driven Development for Embedded C>> and adopts cpputest framework.    
本目录下的VS2019工程（选择DEBUG-X86模式运行）用于对can_iso_tp代码进行单元测试，测试方法参考书籍《测试驱动的嵌入式C语言开发》并采用CPPUTEST框架。

Unit test includes basic, single frame, multi frame, communication between multiple nodes. Through the way of simulation bus, the possible correct / fault operation state can be produced, thus assisting code development and reconstruction.     
单元测试内容包括基础、单帧、多帧、多个节点之间的通信，通过模拟总线的方式，制造出可能的正确/故障运行状态，从而辅助代码开发与重构。

The complete running time of the test case is about 1 minute (AMD 2700x).
测试用例完整运行时间为1分钟左右（AMD 2700X）。

        
       
