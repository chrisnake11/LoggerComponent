#pragma once
#include <string>
// 将日志消息添加不同的级别
enum class LogLevel {
	DEBUG,
	INFO,
	WARNING,
	ERROR,
};

struct LogMessage {
	LogLevel level;
	std::string message;
	std::string timestamp;
};