#pragma once
#include "ILogSink.h"
#include <boost/asio.hpp>
#include <deque>

class NetworkSink : public ILogSink {
public:
	
	explicit NetworkSink(const std::string& ip_address, int port);

	~NetworkSink() override;

	// 从AsioIOContextPool中获取io_context引用，实现异步日志发送
	void log(LogLevel level, const std::string& message, const std::string& timestamp) override;


private:
	// boost网络io库
	boost::asio::io_context& m_io_context;
	boost::asio::ip::tcp::socket m_socket;

	std::string m_server_ip;
	int m_server_port;

	std::deque<std::shared_ptr<std::string>> m_send_buffers;
	std::mutex m_send_mutex;

	bool m_is_writing;

	void connectToServer(const std::string& ip_address, int port);

	void sendAsync(const std::string& message);
	void handleWrite(const boost::system::error_code& error, std::size_t bytes_transfered);
};