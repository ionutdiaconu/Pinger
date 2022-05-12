#pragma once

#include "Session.hpp"

using boost::asio::ip::tcp;
using boost::asio::io_context;


class Server
{

private:
	tcp::acceptor acceptor;

public:
	Server(io_context& io_context, short port);

private:
	void do_accept();
	
};