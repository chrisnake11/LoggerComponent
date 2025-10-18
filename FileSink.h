#pragma once
#include "ILogSink.h"
#include "LogMessage.h"
#include <fstream>
#include <filesystem>
#include <vector>

class FileSink : public ILogSink {
public:
	// 构造函数，接受日志文件路径，打开日志文件
	explicit FileSink(const std::string& file_path);

	// 析构函数，负责关闭日志文件
	~FileSink() override;

	// 执行单条日志写入操作
	void log(LogLevel level, const std::string& message, const std::string& timestamp) override;

	// 执行批量日志写入操作
	void logBatch(const std::vector<LogMessage>& messages) override;

	static constexpr std::size_t FLUSH_THRESHOLD = 100; // 每写入100条日志刷新一次文件缓冲区

private:
	// 创建日志目录（如果不存在）
	void createLogDirectory();
	// 将日志级别转换为字符串表示
	std::string getLevelStr(LogLevel level);
	

private:
	std::string m_file_path;
	std::ofstream m_ofs;
	std::size_t m_flush_count = 0;
	std::size_t m_file_sequence = 0;
};