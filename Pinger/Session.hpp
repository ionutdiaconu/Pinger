#pragma once

#include "DBManager.hpp"

#include <boost/asio.hpp>


using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>
{

private:
	tcp::socket socket;
	DBManager dbManager{};

	enum { max_length = 1024 };
	char data[max_length];

public:
	Session(tcp::socket socket);
	void start();

private:
	void do_read();
	void do_write();

};
