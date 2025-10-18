#include "Logger.h"

std::size_t Logger::m_max_length = 1024;

Logger::Logger(): m_b_stop(false), m_min_level(0) {
	// 初始化sinks
	addSink(std::make_shared<FileSink>("log.txt"));


	// 初始化双缓冲指针
	m_producer_buffer = std::make_unique<std::vector<LogMessage>>(m_producer_queue);
	m_consumer_buffer = std::make_unique<std::vector<LogMessage>>(m_consumer_queue);

	// 启动一个线程，作为消费者，读取队列中的消息。
	m_log_thread = std::thread([this]() {
		while (true) {
			// 等待条件变量通知，或者超时
			{
				// 锁定交换缓冲区的互斥锁
				std::unique_lock<std::mutex> lock(m_queue_mutex);
				// 等待条件变量
				m_consumer_cond.wait(lock, [this]() {
					// 当队列不为空，或者停止后继续（队列为空且没停止，则等待）
					return !m_producer_buffer->empty() || m_b_stop;
					}
				);
			
				std::swap(m_producer_buffer, m_consumer_buffer); // 交换双缓冲指针

				// 通知生产者线程，可以继续生产数据
				m_producer_cond.notify_all();

				// 如果停止标志被设置，并且consumer buffer为空，则退出线程
				if(m_b_stop && m_consumer_buffer->empty() && m_producer_buffer->empty()) {
					break;
				}

			}

			// 从consumer buffer中批量拷贝数据。
			// consumer buffer由消费者线程独占，在while循环中顺序执行处理数据。
			// consumer buffer不需要加锁。
			if (!m_consumer_buffer->empty()) {
				// 遍历所有的日志输出策略接口，调用其logBatch方法
				std::unique_lock<std::mutex> lock(m_sinks_mutex);
				for (const auto& sink : m_log_sinks) {
					if (sink) {
						sink->logBatch(*m_consumer_buffer);
					}
				} 
			}

			// 清空consumer buffer
			m_consumer_buffer->clear();
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
	m_consumer_cond.notify_all();

	// 等待线程退出
	if (m_log_thread.joinable()) {
		m_log_thread.join();
	}
}

/*
	多生产者和单消费者模型下，生产者速度过快，导致的OOM问题。

	虽然consumer vector会与producer vector交换数据，并且在消费者线程中处理数据。
	但是由于生产者线程可能非常频繁地向队列中添加数据，如果消费者线程处理速度跟不上，就会导致
	producer buffer不断增长，最终耗尽内存。

	解决方法：
	1. 限制producer buffer的最大长度，当达到阈值时，阻塞生产者线程，等待消费者线程处理数据。
	2. 当producer buffer达到最大长度的2倍时，丢弃最早的一部分日志消息，保证内存使用在可控范围内。
	3. 通过生产者信号，阻塞生产者线程，等待消费者线程处理数据。

	我们采用第3种方法。

*/

// 生产者
void Logger::pushToQueue(const LogMessage& message) {
	// 选择当前使用的缓冲区
	std::unique_lock<std::mutex> lock(m_queue_mutex);

	// 当producer buffer达到最大长度的2倍时，阻塞生产者线程
	m_producer_cond.wait(lock, [this]() {
		return m_producer_buffer->size() < m_max_length * 2;
		});

	m_producer_buffer->push_back(message);

	// 当缓冲区满时，通知消费者线程
	if (m_producer_buffer->size() >= m_max_length)
		m_consumer_cond.notify_one();
}

// 重载右值版本，避免不必要的拷贝
void Logger::pushToQueue(LogMessage&& message) {
	// 选择当前使用的缓冲区
	std::unique_lock<std::mutex> lock(m_queue_mutex);

	// 当producer buffer达到最大长度的2倍时，阻塞生产者线程
	m_producer_cond.wait(lock, [this]() {
		return m_producer_buffer->size() < m_max_length * 2;
		});

	// 将消息添加到producer buffer中
	m_producer_buffer->push_back(std::move(message));

	// 当缓冲区满时，通知消费者线程
	if (m_producer_buffer->size() >= m_max_length)
		m_consumer_cond.notify_one();
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

