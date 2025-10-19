#include "NetworkSink.h"
#include "AsioIOServicePool.h"

// 将socket绑定到io_context上。
NetworkSink::NetworkSink(const std::string& ip_address, int port)
	: m_server_ip(ip_address), m_server_port(port), m_is_writing(false), m_io_context(AsioIOServicePool::getInstance()->GetIOService()), m_socket(m_io_context)

{
	connectToServer(ip_address, port);
}

NetworkSink::~NetworkSink()
{
	if (m_socket.is_open()) {
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		m_socket.close();
	}

}

void NetworkSink::log(LogLevel level, const std::string& message, const std::string& timestamp)
{
	// 格式化日志消息
	std::ostringstream oss;
	oss << "[" << timestamp << "] ";

	switch (level) {
		case LogLevel::LOG_DEBUG: oss << "[DEBUG] "; break;
		case LogLevel::LOG_INFO: oss << "[INFO] "; break;
		case LogLevel::LOG_WARNING: oss << "[WARNING] "; break;
		case LogLevel::LOG_ERROR: oss << "[ERROR] "; break;
		default: oss << "[UNKNOWN] "; break;
	}

	oss << message << "\n";

	// 异步发送日志消息
	sendAsync(oss.str());
}

void NetworkSink::logBatch(const std::vector<LogMessage>& messages)
{
	std::cout << "NetworkSink::logBatch" << std::endl;
}

void NetworkSink::connectToServer(const std::string& ip_address, int port)
{
	try {
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip_address), port);

		boost::system::error_code ec;
		m_socket.connect(endpoint, ec);
		if (ec) {
			throw std::runtime_error("Failed to connect to server: " + ec.message());
		}
	}
	catch(const std::exception& e) {
		// Handle exception
		std::cout << "Exception in connectToServer: " << e.what() << std::endl;
	}
}

void NetworkSink::sendAsync(const std::string& message)
{
	// 创建共享指针，确保缓冲区在异步操作完成前不会被销毁
	auto buffer = std::make_shared<std::string>(message);
	std::unique_lock<std::mutex> lock(m_send_mutex);
	m_send_buffers.push_back(buffer);

	if (!m_is_writing && m_socket.is_open()) {
		m_is_writing = true;
		auto current_buffer = m_send_buffers.front();
		m_send_buffers.pop_front();

		boost::asio::async_write(m_socket, boost::asio::buffer(*current_buffer),
			[this, current_buffer](const boost::system::error_code& error, std::size_t bytes_transfered) {
				handleWrite(error, bytes_transfered);
			});
	}
}

// 发送后的回调函数。
void NetworkSink::handleWrite(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	std::lock_guard<std::mutex> lock(m_send_mutex);

	if (error) {
		// 处理写入错误，可以尝试重连或记录错误
		m_is_writing = false;

		// 清空队列或尝试重连
		while (!m_send_buffers.empty()) {
			m_send_buffers.pop_front();
		}

		return;
	}

	// 如果队列中还有数据，继续发送
	if (!m_send_buffers.empty()) {
		auto current_buffer = m_send_buffers.front();
		m_send_buffers.pop_front();

		boost::asio::async_write(
			m_socket,
			boost::asio::buffer(*current_buffer),
			[this, current_buffer](const boost::system::error_code& error, std::size_t bytes_transferred) {
				handleWrite(error, bytes_transferred);
			}
		);
	}
	else {
		m_is_writing = false;
	}
}