#include <iostream>



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

using namespace std::chrono_literals;

namespace boost_chrono = boost::asio::chrono;


DBManager dbManager{};

class Pinger
{

private:
	
	std::string pingAddress;

public:
	Pinger(io_context& io_context, const char* destination) :
		icmp_resolver(io_context),
		socket(io_context, icmp::v4()),
		timer(io_context),
		sequence_number(0),
		num_replies(0)
	{
		pingAddress = destination;
		endpoint_destination = *icmp_resolver.resolve(icmp::v4(), destination, "").begin();

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
		echo_request.sequence_number(++sequence_number);
		compute_checksum(echo_request, body.begin(), body.end());

		// Encode the request packet.
		boost::asio::streambuf request_buffer;
		std::ostream os(&request_buffer);
		os << echo_request << body;

		// Send the request.
		time_sent = steady_timer::clock_type::now();
		socket.send_to(request_buffer.data(), endpoint_destination);

		// Wait up to 24 hours for a reply.
		num_replies = 0;
		timer.expires_at(time_sent + boost_chrono::hours(24));
		timer.async_wait(boost::bind(&Pinger::handle_timeout, this));
	}

	void handle_timeout()
	{
		if (num_replies == 0)
		{
			std::cout << "Request timed out" << std::endl;
		}

		// Requests must be sent no less than one second apart.
		timer.expires_at(time_sent + boost_chrono::seconds(1));
		timer.async_wait(boost::bind(&Pinger::start_send, this));
	}

	void start_receive()
	{
		// Discard any data already in the buffer.
		reply_buffer.consume(reply_buffer.size());

		// Wait for a reply. We prepare the buffer to receive up to 64KB.
		socket.async_receive(reply_buffer.prepare(65536),
			boost::bind(&Pinger::handle_receive, this, boost::placeholders::_2));
	}

	void handle_receive(std::size_t length)
	{
		// The actual number of bytes received is committed to the buffer so that we
		// can extract it using a std::istream object.
		reply_buffer.commit(length);

		// Decode the reply packet.
		std::istream is(&reply_buffer);
		ipv4_header ipv4_hdr;
		icmp_header icmp_hdr;
		is >> ipv4_hdr >> icmp_hdr;

		// We can receive all ICMP packets received by the host, so we need to
		// filter out only the echo replies that match the our identifier and
		// expected sequence number.
		if (is && icmp_hdr.type() == icmp_header::echo_reply
			&& icmp_hdr.identifier() == get_identifier()
			&& icmp_hdr.sequence_number() == sequence_number)
		{
			// If this is the first reply, interrupt the five second timeout.
			if (num_replies++ == 0)
				timer.cancel();

			// Print out some information about the reply packet.
			boost_chrono::steady_clock::time_point now = boost_chrono::steady_clock::now();
			boost_chrono::steady_clock::duration elapsed = now - time_sent;

			std::string time = std::to_string(boost_chrono::duration_cast<boost_chrono::milliseconds>(elapsed).count());

			std::cout << length - ipv4_hdr.header_length()
				<< " bytes from " << ipv4_hdr.source_address()
				<< ": icmp_seq=" << icmp_hdr.sequence_number()
				<< ", ttl=" << ipv4_hdr.time_to_live()
				<< ", time="
				<< time
				<< std::endl;

			//TODO: uncomment
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

	icmp::resolver icmp_resolver;
	icmp::endpoint endpoint_destination;
	icmp::socket socket;
	steady_timer timer;
	unsigned short sequence_number;
	boost_chrono::steady_clock::time_point time_sent;
	boost::asio::streambuf reply_buffer;
	std::size_t num_replies;
};


class Session : public std::enable_shared_from_this<Session>
{
public:
	Session(tcp::socket socket) : socket(std::move(socket))
	{
	}

	void start()
	{
		cout << "Starting session" << endl;
		do_read();
	}

private:
	void do_read()
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

	void do_write()
	{
		cout << "Session::do_write" << endl;

		auto self(shared_from_this());

		std::vector<std::string> results = dbManager.getPingDates();

		std::stringstream s;
		for (auto const &result : results)
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

	tcp::socket socket;
	enum { max_length = 1024 };
	char data[max_length];
};

class Server
{
public:
	Server(io_context& io_context, short port) : acceptor(io_context, tcp::endpoint(tcp::v4(), port))
	{
		do_accept();
	}

private:
	void do_accept()
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

	tcp::acceptor acceptor;
};


void startPinger(const char* address)
{
	try
	{
		io_context io_context;
		Pinger p(io_context, address);
		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
}


void startServer()
{
	try
	{
		io_context io_context;
		Server s(io_context, 4444);

		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
}



std::thread serverThread;
std::thread pingerThread;


void stop()
{

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

		io_context serverIOContext;
		io_context pingerIOContext;

		steady_timer timer1{ serverIOContext, std::chrono::seconds{3} };
		timer1.async_wait([](const error_code& ec)
			{
				if (ec)
				{
					std::cerr << "error_code: " << ec.what() << std::endl;
				}

				startServer();
			});

		steady_timer timer2{ pingerIOContext, std::chrono::seconds{3} };
		timer2.async_wait([=](const error_code& ec)
			{
				if (ec)
				{
					std::cerr << "error_code: " << ec.what() << std::endl;
				}

				startPinger(argv[1]);
			});

		
		serverThread = std::thread([&serverIOContext]() { serverIOContext.run(); });
		pingerThread = std::thread([&pingerIOContext]() { pingerIOContext.run(); });
		serverThread.join();

		cout << "Server started! \n";
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}


