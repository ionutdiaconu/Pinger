#pragma once

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include "icmp_header.hpp"
#include "ipv4_header.hpp"

using std::string;
using boost::asio::ip::icmp;
using boost::asio::steady_timer;
using boost::asio::io_context;

namespace boost_chrono = boost::asio::chrono;

class Pinger
{

private:
	
	string pingAddress;
	icmp::resolver icmp_resolver;
	icmp::endpoint endpoint_destination;
	icmp::socket socket;
	steady_timer timer;
	unsigned short sequence_number;
	boost_chrono::steady_clock::time_point time_sent;
	boost::asio::streambuf reply_buffer;
	std::size_t num_replies;

public:
	Pinger(io_context& io_context, const char* destination);

private:
	void start_send();
	void handle_timeout();
	void start_receive();
	void handle_receive(std::size_t length);
	
};

static unsigned short get_identifier()
{
#if defined(BOOST_ASIO_WINDOWS)
	return static_cast<unsigned short>(::GetCurrentProcessId());
#else
	return static_cast<unsigned short>(::getpid());
#endif
}