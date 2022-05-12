#pragma once

#include "Pinger.hpp"
#include <iostream>

using std::cout;
using std::endl;
using boost::asio::io_service;
using boost::asio::ip::tcp;
using boost::asio::async_write;
using boost::system::error_code;
using namespace std::chrono_literals;


Pinger::Pinger(io_context& io_context, const char* destination) :
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


	void Pinger::start_send()
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

	void Pinger::handle_timeout()
	{
		if (num_replies == 0)
		{
			std::cout << "Request timed out" << std::endl;
		}

		// Requests must be sent no less than one second apart.
		timer.expires_at(time_sent + boost_chrono::seconds(1));
		timer.async_wait(boost::bind(&Pinger::start_send, this));
	}

	void Pinger::start_receive()
	{
		// Discard any data already in the buffer.
		reply_buffer.consume(reply_buffer.size());

		// Wait for a reply. We prepare the buffer to receive up to 64KB.
		socket.async_receive(reply_buffer.prepare(65536),
			boost::bind(&Pinger::handle_receive, this, boost::placeholders::_2));
	}

	void Pinger::handle_receive(std::size_t length)
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


	

