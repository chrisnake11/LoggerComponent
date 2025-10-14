#pragma once
#include "ILogSink.h"
#include <fstream>
#include <filesystem>

class FileSink : public ILogSink {
public:
	// 构造函数，接受日志文件路径，打开日志文件
	explicit FileSink(const std::string& file_path);

	// 析构函数，负责关闭日志文件
	~FileSink() override;

	// 执行日志写入操作
	void log(LogLevel level, const std::string& message, const std::string& timestamp) override;

private:
	// 创建日志目录（如果不存在）
	void createLogDirectory();
	// 将日志级别转换为字符串表示
	std::string getLevelStr(LogLevel level);
	

private:
	std::string m_file_path;
	std::ofstream m_ofs;
};