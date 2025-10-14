#pragma once
#include "Singleton.h"
#include <mutex>
#include <condition_variable>
#include <deque>
#include <string>
#include <sstream>
#include <fstream>
#include <thread>
#include <vector>
#include <fmt/format.h>
#include <chrono>
#include "LogLevel.h"
#include "ILogSink.h"
#include "FileSink.h"


// 辅助函数，将单个参数转换为字符串
template<typename T>
std::string to_string_helper(T&& arg) {
	std::ostringstream oss;
	oss << std::forward<T>(arg);
	return oss.str();
}

struct LogMessage {
	LogLevel level;
	std::string message;
	std::string timestamp;
};

/*
	基于单例模式的日志多生产者单消费者的异步日志系统。
	
	生产者将日志消息放入队列，消费者线程从队列中取出消息并写入文件。消息队列有最大长度限制，超过限制时丢弃旧消息。

	使用示例：Logger::getInstance()->log(LogLevel::INFO, "Hello {} {}", "world", 123);
*/
class Logger : public Singleton<Logger> {
	friend class Singleton<Logger>;
public:
	~Logger();

	// 记录日志消息，使用格式化字符串和参数列表。
	template<typename... Args>
	void log(LogLevel level, const std::string& format, Args&&... args);

	virtual void setMinLogLevel(int level) {
		m_min_level = level; 
	}

protected:
	Logger();

private:
	static std::ofstream ofs;

	// 将日志消息构成结构体添加到队列
	void pushToQueue(const LogMessage&& message);
	void pushToQueue(const LogMessage& message);

	void addSink(std::shared_ptr<ILogSink> sink);
	void clearSinks();


	// 基于变长参数化模板的字符串格式输出函数。
	// 按照格式化字符串和参数列表，并生成日志消息。
	template<typename... Args>
	std::string formatMessage(const std::string& format, Args&&...args);

	std::string getCurrentTimeString() {
		std::time_t now_time = std::time(nullptr);  // 获取当前时间戳
		if (now_time == -1) {  // 处理time()可能的失败
			return "Failed to get time";
		}

		std::tm tm_buf{};  // 初始化tm结构体（清零）
		// 调用localtime_s：第一个参数是tm指针，第二个是time_t指针
		errno_t err = localtime_s(&tm_buf, &now_time);
		if (err != 0) {  // 检查错误（非0表示失败）
			return "Failed to convert time";
		}

		char buffer[20];  // 足够存储"YYYY-MM-DD HH:MM:SS"
		// 格式化时间到buffer
		std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_buf);
		return std::string(buffer);
	}

	std::atomic<bool> m_b_stop;
	std::condition_variable m_cond;
	std::mutex m_queue_mutex;
	std::deque<LogMessage> m_message_queue;
	std::thread m_log_thread;
	const size_t m_max_length;
	int m_min_level; // 最小日志级别

	std::deque<std::shared_ptr<ILogSink>> m_log_sinks; // 日志输出策略接口的列表
	std::mutex m_sinks_mutex; // 保护 m_log_sinks 访问的互斥锁

};

template<typename ...Args>
inline void Logger::log(LogLevel level, const std::string& format, Args && ...args)
{
	std::string timestamp = getCurrentTimeString();
	std::string message = formatMessage(format, std::forward<Args>(args)...);

	// 拼接日志级别和格式化后的消息,以及当前时间
	pushToQueue(LogMessage{level, std::move(message), std::move(timestamp)});
}

template<typename ...Args>
inline std::string Logger::formatMessage(const std::string& format, Args && ...args)
{
	try {
		// 尝试使用 fmt 库进行格式化
		return fmt::format(format, std::forward<Args>(args)...);
	}
	catch(const fmt::format_error& e) {
		// 如果格式化失败，采用简单的拼接方式
		std::vector<std::string> arg_strings = { to_string_helper(std::forward<Args>(args))... };
		std::ostringstream oss;
		std::size_t arg_index = 0;
		std::size_t pos = 0;
		std::size_t placeholder_pos = format.find("{}", pos);

		while (placeholder_pos != std::string::npos) {
			// 将占位符前的部分添加到输出流
			oss << format.substr(pos, placeholder_pos - pos);
			if (arg_index < arg_strings.size()) {
				// 添加对应的参数字符串
				oss << arg_strings[arg_index++];
			}
			else {
				oss << "{}"; // 如果参数不足，保留占位符
			}

			pos = placeholder_pos + 2; // 移动到下一个占位符位置
			placeholder_pos = format.find("{}", pos); // 查找下一个占位符
		}
		// 添加剩余的字符串部分
		oss << format.substr(pos);

		// 处理额外的参数
		while (arg_index < arg_strings.size()) {
			oss << " " << arg_strings[arg_index++];
		}

		return oss.str();
	}
	
}
