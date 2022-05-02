#include <iostream>

using std::cout;
using std::endl;
using std::string;

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <istream>
#include <iostream>
#include <ostream>

#include "icmp_header.hpp"
#include "ipv4_header.hpp"
#include "DBManager.h"

#include <thread>
#include <atomic>
#include <memory>

#include <cstdlib>
#include <utility>

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
using namespace boost::asio;


using namespace std::chrono_literals;
using namespace boost;

using boost::asio::ip::icmp;
using boost::asio::steady_timer;
namespace chrono = boost::asio::chrono;

class pinger
{

private:
	DBManager dbManager{};
	std::string pingAddress;

public:
	pinger(boost::asio::io_context& io_context, const char* destination)
		: resolver_(io_context), socket_(io_context, icmp::v4()),
		timer_(io_context), sequence_number_(0), num_replies_(0)
	{
		pingAddress = destination;
		destination_ = *resolver_.resolve(icmp::v4(), destination, "").begin();

		start_send();
		start_receive();
	}

private:
	void start_send()
	{
		std::string body("\"hearthbeat monitor.");

		// Create an ICMP header for an echo request.
		icmp_header echo_request;
		echo_request.type(icmp_header::echo_request);
		echo_request.code(0);
		echo_request.identifier(get_identifier());
		echo_request.sequence_number(++sequence_number_);
		compute_checksum(echo_request, body.begin(), body.end());

		// Encode the request packet.
		boost::asio::streambuf request_buffer;
		std::ostream os(&request_buffer);
		os << echo_request << body;

		// Send the request.
		time_sent_ = steady_timer::clock_type::now();
		socket_.send_to(request_buffer.data(), destination_);

		// Wait up to five seconds for a reply.
		num_replies_ = 0;
		timer_.expires_at(time_sent_ + chrono::seconds(5));
		timer_.async_wait(boost::bind(&pinger::handle_timeout, this));
	}

	void handle_timeout()
	{
		if (num_replies_ == 0)
			std::cout << "Request timed out" << std::endl;

		// Requests must be sent no less than one second apart.
		timer_.expires_at(time_sent_ + chrono::seconds(1));
		timer_.async_wait(boost::bind(&pinger::start_send, this));
	}

	void start_receive()
	{
		// Discard any data already in the buffer.
		reply_buffer_.consume(reply_buffer_.size());

		// Wait for a reply. We prepare the buffer to receive up to 64KB.
		socket_.async_receive(reply_buffer_.prepare(65536),
			boost::bind(&pinger::handle_receive, this, boost::placeholders::_2));
	}

	void handle_receive(std::size_t length)
	{
		// The actual number of bytes received is committed to the buffer so that we
		// can extract it using a std::istream object.
		reply_buffer_.commit(length);

		// Decode the reply packet.
		std::istream is(&reply_buffer_);
		ipv4_header ipv4_hdr;
		icmp_header icmp_hdr;
		is >> ipv4_hdr >> icmp_hdr;

		// We can receive all ICMP packets received by the host, so we need to
		// filter out only the echo replies that match the our identifier and
		// expected sequence number.
		if (is && icmp_hdr.type() == icmp_header::echo_reply
			&& icmp_hdr.identifier() == get_identifier()
			&& icmp_hdr.sequence_number() == sequence_number_)
		{
			// If this is the first reply, interrupt the five second timeout.
			if (num_replies_++ == 0)
				timer_.cancel();

			// Print out some information about the reply packet.
			chrono::steady_clock::time_point now = chrono::steady_clock::now();
			chrono::steady_clock::duration elapsed = now - time_sent_;

			std::string time = std::to_string(chrono::duration_cast<chrono::milliseconds>(elapsed).count());

			std::cout << length - ipv4_hdr.header_length()
				<< " bytes from " << ipv4_hdr.source_address()
				<< ": icmp_seq=" << icmp_hdr.sequence_number()
				<< ", ttl=" << ipv4_hdr.time_to_live()
				<< ", time="
				<< time
				<< std::endl;

			
			//dbManager.insert(pingAddress,time);
		}

		start_receive();
	}

	static unsigned short get_identifier()
	{
		#if defined(BOOST_ASIO_WINDOWS)
			return static_cast<unsigned short>(::GetCurrentProcessId());
		#else
			return static_cast<unsigned short>(::getpid());
		#endif
	}

	icmp::resolver resolver_;
	icmp::endpoint destination_;
	icmp::socket socket_;
	steady_timer timer_;
	unsigned short sequence_number_;
	chrono::steady_clock::time_point time_sent_;
	boost::asio::streambuf reply_buffer_;
	std::size_t num_replies_;
};


void SendHandler(boost::system::error_code ex) {
	std::cout << " do something here" << std::endl;
}



using asio::ip::tcp;

class session
	: public std::enable_shared_from_this<session>
{
public:
	session(tcp::socket socket)
		: socket_(std::move(socket))
	{
	}

	void start()
	{
		do_read();
	}

private:
	void do_read()
	{

		cout << "do_read" << endl;

		auto self(shared_from_this());
		socket_.async_read_some(asio::buffer(data_, max_length),
			[this, self](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					do_write(length);
				}
			});
	}

	void do_write(std::size_t length)
	{


		cout << "do_write" << endl;

		auto self(shared_from_this());
		asio::async_write(socket_, asio::buffer(data_, length),
			[this, self](std::error_code ec, std::size_t /*length*/)
			{
				if (!ec)
				{
					do_read();
				}
			});
	}

	tcp::socket socket_;
	enum { max_length = 1024 };
	char data_[max_length];
};

class server
{
public:
	server(asio::io_context& io_context, short port)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
	{
		do_accept();
	}

private:
	void do_accept()
	{
		cout << "do_accept" << endl;

		acceptor_.async_accept(
			[this](std::error_code ec, tcp::socket socket)
			{
				if (!ec)
				{
					std::make_shared<session>(std::move(socket))->start();
				}

				do_accept();
			});
	}

	tcp::acceptor acceptor_;
};



void startPinger(const char* address)
{
	boost::asio::io_context io_context;
	pinger p(io_context, address);
	io_context.run();
}


void startServer()
{
	try
	{	
		asio::io_context io_context;
		server s(io_context, 4444);

		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
}


int main(int argc, char* argv[])
{
	try
	{
		if (argc != 2)
		{
			std::cerr << "Usage: ping <host>" << std::endl;
			#if !defined(BOOST_ASIO_WINDOWS)
				std::cerr << "(You may need to run this program as root.)" << std::endl;
			#endif
			return 1;
		}

		io_service ioservice1;
		io_service ioservice2;

		steady_timer timer1{ ioservice1, std::chrono::seconds{3} };
		timer1.async_wait([](const boost::system::error_code& ec)
		{ 
			startServer();
		});

		steady_timer timer2{ ioservice2, std::chrono::seconds{3} };
		timer2.async_wait([=](const boost::system::error_code& ec)
		{ 
			startPinger(argv[1]);
		});

		std::thread thread1{ [&ioservice1]() { ioservice1.run(); } };
		std::thread thread2{ [&ioservice2]() { ioservice2.run(); } };
		thread1.join();
		thread2.join();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}


