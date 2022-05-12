#include "Session.hpp"

#include <istream>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>

using std::cout;
using std::endl;
using std::string;

using boost::asio::ip::icmp;
using boost::asio::steady_timer;
using boost::asio::ip::tcp;
using boost::asio::io_service;
using boost::asio::io_context;
using boost::asio::async_write;
using boost::system::error_code;


Session::Session(tcp::socket socket) : socket(std::move(socket))
{

}

void Session::start()
{
	cout << "Starting session" << endl;
	do_read();
}


void Session::do_read()
{
	cout << "Session::do_read" << endl;

	/*make sure that connection object outlives the asynchronous operation;
	as long as the lambda is alive (the async. operation is in progress),
	the connection instance is alive as well.*/
	auto self(shared_from_this());
	socket.async_read_some(boost::asio::buffer(data, max_length),
		[this, self](std::error_code ec, std::size_t length)
		{
			if (!ec)
			{
				do_write();
			}
		});
}

void Session::do_write()
{
	cout << "Session::do_write" << endl;

	auto self(shared_from_this());

	std::vector<std::string> results = dbManager.getPingDates();

	std::stringstream s;
	for (auto const& result : results)
	{
		s << result << "; ";
	}

	std::string ss = s.str();
	const char* dataToSend = ss.c_str();

	auto dataToSendSize = std::strlen(dataToSend);

	cout << "sending client response: " << dataToSend << " len:" << dataToSendSize << endl;

	async_write(socket, boost::asio::buffer(dataToSend, dataToSendSize),
		[this, self](std::error_code ec, std::size_t /*length*/)
		{
			if (!ec)
			{
				do_read();
			}
		});
}

