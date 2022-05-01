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

			
			dbManager.insert(pingAddress,time);
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





//responsible for handling a single client by reading the request message, processing it, and then sending back the response message.
//Each instance of the Service class is intended to handle one connected client
//by reading the request message, processing it, and then sending the response message back.
class Service
{
public:
    //The class's constructor accepts a shared pointer to an object representing a socket connected to a particular client as an argument
    // and caches this pointer. This socket will be used later to communicate with the client application.
    Service(std::shared_ptr<asio::ip::tcp::socket> sock) : m_sock(sock)
    {
    }

    //This method starts handling the client by initiating the asynchronous reading operation
    //to read the request message from the client specifying the onRequestReceived() method as a callback.
    void StartHandling()
    {

        asio::async_read_until(*m_sock.get(),
            m_request,
            '\n',
            [this](
                const boost::system::error_code& ec,
                std::size_t bytes_transferred)
            {
                //When the request reading completes, or an error occurs, the callback method onRequestReceived() is called.
                onRequestReceived(ec,
                    bytes_transferred);
            });
    }

private:
    void onRequestReceived(const boost::system::error_code& ec,
        std::size_t bytes_transferred)
    {
        //This method first checks whether the reading succeeded by testing the ec argument that contains the operation completion status code.
        if (ec.value() != 0)
        {
            std::cout << "Error occured! Error code = "
                << ec.value()
                << ". Message: " << ec.message();
            //reading finished with an error, the corresponding message is output to the standard output stream
            //and then the onFinish() method is called.
            onFinish();
            return;
        }

        // Process the request.
        m_response = ProcessRequest(m_request);

        // When the ProcessRequest() method completes and returns the string containing the response message,
        // the asynchronous writing operation is initiated to send this response message back to the client.
        asio::async_write(*m_sock.get(),
            asio::buffer(m_response),
            [this](
                const boost::system::error_code& ec,
                std::size_t bytes_transferred)
            {
                //The onResponseSent() method is specified as a callback.
                onResponseSent(ec, bytes_transferred);
            });
    }

    void onResponseSent(const boost::system::error_code& ec,
        std::size_t bytes_transferred)
    {
        // This method first checks whether the operation succeeded.
        if (ec.value() != 0)
        {
            // If the operation failed, the corresponding message is output to the standard output stream.
            std::cout << "Error occured! Error code = "
                << ec.value()
                << ". Message: " << ec.message();
        }

        //method is called to perform the cleanup.
        onFinish();
    }

    // Here we perform the cleanup.
    void onFinish()
    {
        delete this;
    }

    //To keep things simple,  we implement a dummy service which only emulates the execution of certain operations
    //The request processing emulation consists of performing many increment operations to emulate operations
    //that intensively consume CPU and then putting the thread of control to sleep for some time to emulate I/O operations
    std::string ProcessRequest(asio::streambuf& request)
    {

        // In this method we parse the request, process it
        // and prepare the request.

        // Emulate CPU-consuming operations.
        int i = 0;
        while (i != 1000000) {
            ++i;
        }

        // Emulate operations that block the thread
        // (e.g. synch I/O operations).
        std::this_thread::sleep_for(
            std::chrono::milliseconds(100));

        // Prepare and return the response message.
        std::string response = "Response from server\n";
        return response;
    }

private:
    std::shared_ptr<asio::ip::tcp::socket> m_sock;
    std::string m_response;
    asio::streambuf m_request;
};

//responsible for accepting the connection requests arriving from clients and instantiating the objects of the Service class,
// which will provide the service to connected clients.
class Acceptor
{
public:
    //Its constructor accepts a port number on which it will listen for the incoming connection requests as its input argument. 
    Acceptor(asio::io_service& ios, unsigned short port_num) : m_ios(ios),
        //The object of this class contains an instance of the asio::ip::tcp::acceptor class as its member named m_acceptor,
        //which is constructed in the Acceptor class's constructor.
        m_acceptor(m_ios,
            asio::ip::tcp::endpoint(
                asio::ip::address_v4::any(),
                port_num)),
        m_isStopped(false)
    {
    }

    //The Start() method is intended to instruct an object of the Acceptor class to start listening and accepting incoming connection requests.
    void Start()
    {
        //It puts the m_acceptor acceptor socket into listening mode
        m_acceptor.listen();
        InitAccept();
    }

    // Stop accepting incoming connection requests.
    void Stop()
    {
        m_isStopped.store(true);
    }

private:
    void InitAccept()
    {
        //constructs an active socket object and initiates the asynchronous accept operation
        std::shared_ptr<asio::ip::tcp::socket>
            sock(new asio::ip::tcp::socket(m_ios));

        //calling the async_accept() method on the acceptor socket object
        // and passing the object representing an active socket to it as an argument.
        m_acceptor.async_accept(*sock.get(),
            [this, sock](
                const boost::system::error_code& error)
            {
                //When the connection request is accepted or an error occurs, the callback method onAccept() is called.
                onAccept(error, sock);
            });
    }

    void onAccept(const boost::system::error_code& ec,
        std::shared_ptr<asio::ip::tcp::socket> sock)
    {
        if (ec.value() == 0)
        {
            //an instance of the Service class is created and its StartHandling() method is called
            (new Service(sock))->StartHandling();
        }
        else
        {
            //the corresponding message is output to the standard output stream.
            std::cout << "Error occured! Error code = "
                << ec.value()
                << ". Message: " << ec.message();
        }

        // Init next async accept operation if
        // acceptor has not been stopped yet.
        if (!m_isStopped.load())
        {
            InitAccept();
        }
        else
        {
            // Stop accepting incoming connections
            // and free allocated resources.
            m_acceptor.close();
        }
    }

private:
    asio::io_service& m_ios;
    //used to asynchronously accept the incoming connection requests.
    asio::ip::tcp::acceptor m_acceptor;
    std::atomic<bool> m_isStopped;
};

//represents the server itself
class Server
{
public:
    Server()
    {
        m_work.reset(new asio::io_service::work(m_ios));
    }

    // Start the server.
    // Accepts a protocol port number on which the server should listen for the incoming connection requests
    // and the number of threads to add to the pool as input arguments and starts the server
    // Nonblocking Method
    void Start(unsigned short port_num,
        unsigned int thread_pool_size)
    {

        assert(thread_pool_size > 0);

        // Create and start Acceptor.
        acc.reset(new Acceptor(m_ios, port_num));
        acc->Start();

        // Create specified number of threads and
        // add them to the pool.
        for (unsigned int i = 0; i < thread_pool_size; i++)
        {
            std::unique_ptr<std::thread> th(
                new std::thread([this]()
                    { m_ios.run(); }));

            m_thread_pool.push_back(std::move(th));
        }
    }

    // Stop the server.
    // Blocks the caller thread until the server is stopped and all the threads running the event loop exit.
    void Stop()
    {
        acc->Stop();
        m_ios.stop();

        for (auto& th : m_thread_pool)
        {
            th->join();
        }
    }

private:
    asio::io_service m_ios;
    std::unique_ptr<asio::io_service::work> m_work;
    std::unique_ptr<Acceptor> acc;
    std::vector<std::unique_ptr<std::thread>> m_thread_pool;
};

const unsigned int DEFAULT_THREAD_POOL_SIZE = 2;

int startServer()
{
    unsigned short port_num = 3333;

    try
    {
        //it instantiates an object of the Server class named srv.
        Server srv;

        //before starting the server, the optimal size of the pool is calculated.
        // The general formula often used in parallel applications to find the optimal number of threads is the number of processors the computer has multiplied by 2.
        // We use the std::thread::hardware_concurrency() static method to obtain the number of processors. 
        unsigned int thread_pool_size =  std::thread::hardware_concurrency() * 2;

        //because this method may fail to do its job returning 0,
        // we fall back to default value represented by the constant DEFAULT_THREAD_POOL_SIZE, which is equal to 2 in our case.
        if (thread_pool_size == 0)
        {
            thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
        }

        srv.Start(port_num, thread_pool_size);

        std::this_thread::sleep_for(std::chrono::seconds(60));

        srv.Stop();
    }
    catch (system::system_error& e)
    {
        std::cout << "Error occured! Error code = "
            << e.code() << ". Message: "
            << e.what();
    }

    return 0;
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
        cout << "starting ping server "  << endl;
        startServer();
        cout << "ping server started." << endl;

		const string pingAddress = argv[1];
		cout << "pinging " << pingAddress << endl;

		//boost::asio::io_context io_context;
		//pinger p(io_context, argv[1]);
		//io_context.run();

       

		
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}


