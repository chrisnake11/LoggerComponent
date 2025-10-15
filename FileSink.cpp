#include "FileSink.h"
#include <stdexcept>
#include <iostream>
FileSink::FileSink(const std::string& file_path) : m_file_path(file_path)
{
	createLogDirectory();
	m_ofs.open(m_file_path, std::ios::app);
	if(!m_ofs.is_open()) {
		throw std::runtime_error("Failed to open log file: " + m_file_path);
	}
}

FileSink::~FileSink()
{
	if(m_ofs.is_open()) {
		m_ofs.close();
	}
}

std::string FileSink::getLevelStr(LogLevel level)
{
	switch(level) {
		case LogLevel::DEBUG: return "[DEBUG]";
		case LogLevel::INFO: return "[INFO]";
		case LogLevel::WARNING: return "[WARNING]";
		case LogLevel::ERROR: return "[ERROR]";
		default: return "[UNKNOWN]";
	}
}

void FileSink::createLogDirectory() {
	// 初始化path
	std::filesystem::path path(m_file_path);
	// 获取父目录路径
	std::filesystem::path dir = path.parent_path();
	// 如果目录为空，表示当前目录，无需创建
	if(dir.empty()) {
		return;
	}
	// 父目录不存在则创建目录
	try {
		if (!std::filesystem::exists(dir)) {
			bool created = std::filesystem::create_directories(dir);
			if(!created) {
				std::cerr << "Warning: Failed to create directory (already exists?): " << dir << std::endl;
			}
		}
	}
	catch(const std::filesystem::filesystem_error& e) {
		std::cerr << "Error creating directory '" << dir << "': " << e.what() << std::endl;
	}
	catch (const std::exception& e) {
		// 捕获其他可能的异常
		std::cerr << "Unexpected error: " << e.what() << std::endl;
	}

}

void FileSink::logBatch(const std::vector<LogMessage>& messages)
{
	// 如果文件关闭则直接返回
	if (!m_ofs.is_open()) {
		return;
	}
	// 输出数据到filestream中
	if (m_ofs.is_open()) {
		// 批量顺序写入
		for (const auto& msg : messages) {
			if(msg.level < m_min_level) {
				continue;
			}
			
			// 格式化写入数据
			m_ofs << "[" << msg.timestamp << "]" << " "
				<< getLevelStr(msg.level) << " "
				<< msg.message << '\n';
		}
		
		// 批量循环写入后，刷新缓冲区
		m_ofs.flush();
	}
}

void FileSink::log(LogLevel level, const std::string& message, const std::string& timestamp)
{
	// 如果文件关闭或者日志级别低于最小级别则直接返回
	if (!m_ofs.is_open() || level < m_min_level) {
		return;
	}
	// 输出数据到filestream中
	if(m_ofs.is_open()) {
		m_ofs << "[" << timestamp << "]" << " "
			<< getLevelStr(level) << " "
			<< message << '\n';
	}
}