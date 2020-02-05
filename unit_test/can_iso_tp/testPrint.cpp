#include <stdio.h>  
#include <stdarg.h>  
#include <string.h>  

#include "testPrint.h"


bool print_enable = false;

void testPrintf(const char *cmd, ...)
{
	if (print_enable)
	{
		va_list args;       //定义一个va_list类型的变量，用来储存单个参数  
		va_start(args, cmd); //使args指向可变参数的第一个参数  
		vprintf(cmd, args);  //必须用vprintf等带V的  
		va_end(args);       //结束可变参数的获取
		int n = strlen(cmd);
		if(n>=1 && cmd[n-1] != '\n')
			printf("\n");
	}
}