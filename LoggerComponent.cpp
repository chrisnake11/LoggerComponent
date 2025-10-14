// LoggerComponent.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "Logger.h"

int main() {
	for (int i = 0; i < 10; i=i+2) {
		Logger::getInstance()->log(LogLevel::INFO, "hello {} world", i);
		Logger::getInstance()->log(LogLevel::DEBUG, "hello {} world", i+1);
	}
}