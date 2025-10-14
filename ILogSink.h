#pragma once
#include "LogLevel.h"
#include <string>

class ILogSink {
public:
	virtual ~ILogSink() = default;

	// log输出接口
	virtual void log(LogLevel level, const std::string& message, const std::string& timestamp) = 0;

	virtual void setMinLogLevel(LogLevel level) {
		m_min_level = level;
	}

protected:
	LogLevel m_min_level = LogLevel::DEBUG;
};

