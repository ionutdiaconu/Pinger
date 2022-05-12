#include "Server.hpp"
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

using boost::asio::async_write;
using boost::system::error_code;
using boost::asio::ip::icmp;
using boost::asio::steady_timer;
using boost::asio::ip::tcp;
using boost::asio::io_service;
using boost::asio::io_context;



Server::Server(io_context& io_context, short port) : acceptor(io_context, tcp::endpoint(tcp::v4(), port))
{
	do_accept();
}


void Server::do_accept()
{
	cout << "Server::do_accept" << endl;

	acceptor.async_accept(
		[this](std::error_code ec, tcp::socket socket)
		{
			if (!ec)
			{
				cout << "[Server] New connection: " << socket.remote_endpoint() << "\n";

				std::make_shared<Session>(std::move(socket))->start();
			}
			else
			{
				cout << "[Server] New connection error: " << ec.message() << "\n";
			}

			do_accept();
		});
}

