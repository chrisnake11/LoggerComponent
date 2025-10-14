#include "Logger.h"


std::ofstream Logger::ofs("log.txt", std::ios::app);

Logger::Logger(): m_b_stop(false), m_max_length(1024), m_min_level(0) {

	// open filestream
	if (!ofs.is_open()) {
		ofs.open("log.txt", std::ios::app);
	}

	// 初始化sinks
	addSink(std::make_shared<FileSink>("log1.txt"));
	addSink(std::make_shared<FileSink>("log2.txt"));
	addSink(std::make_shared<FileSink>("log3.txt"));

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
			LogMessage m_log_message = m_message_queue.front();
			m_message_queue.pop_front();

			// 将消息写入所有的日志输出接口
			for(std::shared_ptr<ILogSink> sink : m_log_sinks) {
				sink->log(m_log_message.level, m_log_message.message, m_log_message.timestamp);
			}

			// 执行写入
			lock.unlock();
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
void Logger::pushToQueue(const LogMessage& message) {
	std::unique_lock<std::mutex> lock(m_queue_mutex);
	if (m_message_queue.size() >= m_max_length)
		m_message_queue.pop_front(); // 将旧的消息删除。
	m_message_queue.push_back(message);
	std::cout << "push message " << message.message << std::endl;
	lock.unlock();
	m_cond.notify_one();
}

void Logger::addSink(std::shared_ptr<ILogSink> sink)
{
	std::unique_lock<std::mutex> lock(m_sinks_mutex);
	m_log_sinks.emplace_back(std::move(sink));
}

void Logger::clearSinks()
{
	std::unique_lock<std::mutex> lock(m_sinks_mutex);
	m_log_sinks.clear();
}

// 重载右值版本，避免不必要的拷贝
void Logger::pushToQueue(const LogMessage&& message) {
	std::unique_lock<std::mutex> lock(m_queue_mutex);
	if (m_message_queue.size() >= m_max_length)
		m_message_queue.pop_front(); // 将旧的消息删除。
	m_message_queue.emplace_back(std::move(message));
	std::cout << "push message " << message.message << std::endl;
	lock.unlock();
	m_cond.notify_one();
}

