#pragma once
#include "LogLevel.h"
struct LogMessage {
	LogLevel level;
	std::string message;
	std::string timestamp;
};