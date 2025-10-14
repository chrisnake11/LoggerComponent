#include "Logger.h"


std::ofstream Logger::ofs("log.txt", std::ios::app);

Logger::Logger(): m_b_stop(false), m_max_length(1024) {

	// open filestream
	if (!ofs.is_open()) {
		ofs.open("log.txt", std::ios::app);
	}

	// 启动一个线程，作为消费者，读取队列中的消息。
	m_log_thread = std::thread([this]() {
		while (true) {
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			m_cond.wait(lock, [this]() {
				// 当队列不为空，或者停止后继续（队列为空且没停止，则等待）
				return !m_message_queue.empty() || m_b_stop;
				}
			);

			// 如果队列为空，并且服务器停止。不在执行
			if (m_message_queue.empty() && m_b_stop) {
				break;
			}

			// 否则，继续读取队列中的消息（正常读取或者清空）
			std::string m_log_message = m_message_queue.front();
			m_message_queue.pop_front();

			// 执行写入
			lock.unlock();
			writeToFile(m_log_message);
		}
	});
}

Logger::~Logger() {
	// 修改停止标志
	{
		std::unique_lock<std::mutex> lock(m_queue_mutex);
		m_b_stop = true;
	}

	// 唤醒消费者线程，等待其退出
	m_cond.notify_all();

	// 等待线程退出
	if (m_log_thread.joinable()) {
		m_log_thread.join();
	}
	// close ofstream
	if (ofs.is_open()) {
		ofs.flush();  // 确保写入落盘
		ofs.close();
	}
}

// 生产者
void Logger::pushToQueue(const std::string& message) {
	std::unique_lock<std::mutex> lock(m_queue_mutex);
	if (m_message_queue.size() >= m_max_length)
		m_message_queue.pop_front(); // 将旧的消息删除。
	m_message_queue.push_back(message);
	std::cout << "push message " << message << std::endl;
	lock.unlock();
	m_cond.notify_one();
}

void Logger::writeToFile(const std::string& message) {
	if (!ofs.is_open()) {
		// 处理打开失败（如输出错误日志、抛出异常）
		std::cout << "log.txt open failed" << std::endl;
		return;
	}
	if (!(ofs << message << std::endl)) {
		// 处理写入失败
		std::cout << "log.txt load failed" << std::endl;
	}
	else {
		std::cout << "write message " << message << std::endl;
	}
}