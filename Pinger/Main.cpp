#include "Server.hpp"
#include "Pinger.hpp"

#include <istream>
#include <iostream>
#include <ostream>

using std::cout;
using std::endl;
using std::string;
using boost::system::error_code;


std::thread serverThread;
std::thread pingerThread;

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


