#include "Logger.h"

Logger::Logger(): m_b_stop(false), m_max_length(1024), m_min_level(0) {
	// 初始化sinks
	addSink(std::make_shared<FileSink>("log.txt"));

	// 初始化消息队列
	m_message_buffer.reserve(m_max_length);

	// 启动一个线程，作为消费者，读取队列中的消息。
	m_log_thread = std::thread([this]() {
		while (true) {
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			// 等待条件变量，或者超时100ms
			m_cond.wait_for(lock, std::chrono::milliseconds(100), [this]() {
				// 当队列不为空，或者停止后继续（队列为空且没停止，则等待）
				return !m_message_queue.empty() || m_b_stop;
				}
			);

			// 如果队列为空，并且服务器停止。不在执行
			if (m_message_queue.empty() && m_b_stop) {
				break;
			}

			// 将队列中的消息全部移动到缓冲区，减少锁持有时间
			while (!m_message_queue.empty()) {
				m_message_buffer.push_back(std::move(m_message_queue.front()));
				m_message_queue.pop_front();
			}

			// 解锁，允许生产者继续添加消息
			lock.unlock();

			// 将buffer缓冲区的消息，批量拷贝到所有的sink
			for(std::shared_ptr<ILogSink> sink : m_log_sinks) {
				sink->logBatch(m_message_buffer);
			}

			m_message_buffer.clear(); // 清空缓冲区，准备下一批消息
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
void Logger::pushToQueue(LogMessage&& message) {
	std::unique_lock<std::mutex> lock(m_queue_mutex);
	if (m_message_queue.size() >= m_max_length)
		m_message_queue.pop_front(); // 将旧的消息删除。
	m_message_queue.emplace_back(std::move(message));
	lock.unlock();
	m_cond.notify_one();
}

