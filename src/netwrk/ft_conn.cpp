//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>
#include <system_error>

// POSIX & LINUX headers
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// App specific headers
#include "ft_utils.hpp"
#include "request/ft_req.hpp"
#include "request/ft_req_brkr.hpp"
#include "netwrk/ft_conn.hpp"
#include "protocol/ft_msg.hpp"
#include "request/ft_req_hndlr.hpp"
#include "netwrk/ft_conn_utils.hpp"


namespace ft { namespace netwrk {

request::RequestBrokerPtr    Connection::sm_request_broker;

Connection::Connection(int fd)
: Pollable(fd)
{
	// This Constructor handles the connection on server side after return by
	// accept()
	try {
		setup_socket_options(fd);
	} catch (std::system_error& syserr) {
		(void)close(fd);
		throw syserr;
	}
}

Connection::Connection(const std::string& host, const uint16_t port)
: Pollable(-1)
{
	// This constructor handles the connection from the client to the server

	struct addrinfo hints;
    struct addrinfo *result;

	// Resolve host address
	(void)memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

	int ret = getaddrinfo(host.c_str(), std::to_string((unsigned)port).c_str(),
			&hints, &result);
	if (ret != 0 || result == NULL) {
		throw std::runtime_error(
				std::string("Failed to resolve host name: ") + host +
				std::string(" - [" + std::to_string(ret) + "]: " +
				gai_strerror(ret)));
	}

	// Try to connect to the returned host IP address until one succeeds
	int tmpfd = -1;
	for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
		tmpfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (tmpfd < 0) {
			continue;
		}

		if (connect(tmpfd, rp->ai_addr, rp->ai_addrlen) >= 0) {
			break;
		}

		std::cout << "Failed to connect: " << errno << std::endl;
		(void)close(tmpfd);
		tmpfd = -1;
	}

	freeaddrinfo(result);

	if (tmpfd < 0) {
		throw std::runtime_error(
				std::string("Failed to connect to host: ") + host +
				std::string(":") + std::to_string(port));
	}

	// If succeeded to connect to the host, setup the socket
	try {
		setup_socket_options(tmpfd);
	} catch (std::system_error& syserr) {
		(void)close(tmpfd);
		throw syserr;
	}

	this->fd = tmpfd;
}

Connection::~Connection()
{
	std::cout << "FT        | connection end" << std::endl;
	if (this->fd >= 0) {
		(void)shutdown(this->fd, SHUT_RDWR);
		(void)close(this->fd);
	}
}

void Connection::handleEvent()
{
	// This is part of the Pollable interface and called from the PollGroup
	// whenever an event is notified for the underlying fd.

	// The protocol has an envelope header containing:
	//    MAGIC:           3 bytes (used for tag the message start)
	//    MESSAGE TYPE:    1 byte
	//    MESSAGE LEN:     2 bytes
	//    MESSAGE PAYLOAD: Variable length

	static const uint8_t MAGIC1 = (uint8_t)((ft::proto::MAGIC >> 24) & 0xff);
	static const uint8_t MAGIC2 = (uint8_t)((ft::proto::MAGIC >> 16) & 0xff);
	static const uint8_t MAGIC3 = (uint8_t)((ft::proto::MAGIC >> 8 ) & 0xff);
	static const uint8_t MAX_MSG_TYPE = (uint8_t)(ft::proto::MSGTYPE_MAX & 0xff);
	uint8_t b;
	int ret;

	// This is a simple state machine parser with a buffer big enough to hold
	// the message.
	// It reads byte per byte from the stream until succeeds to parse the
	// message. While parsing the header, if it identifies an inconsistency in
	// the header it discards the read bytes and starts all over again.
	//
	do {
		int ret = recv(this->fd, &b, 1, 0);
		if(ret > 0) {
			size_t buf_curr_size = this->msg_buf.size();
			if (buf_curr_size == 0U && b == MAGIC1) {
				this->msg_buf.push_back(b); // MAGIC first byte
			} else if(buf_curr_size == 1U && b == MAGIC2) {
				this->msg_buf.push_back(b); // MAGIC second byte
			} else if(buf_curr_size == 2U && b == MAGIC3) {
				this->msg_buf.push_back(b); // MAGIC third byte
			} else if(buf_curr_size == 3U && b > 0U && b < MAX_MSG_TYPE) {
				this->msg_buf.push_back(b); // Message type
			} else if(buf_curr_size == 4U) {
				this->msg_buf.push_back(b); // Message length MSB
				this->msg_len = (((uint16_t)b) << 8) & 0xff00;
			} else if(buf_curr_size == 5U) {
				this->msg_buf.push_back(b); // Message length LSB
				this->msg_len |= ((uint16_t)b) & 0x00ff;
			} else if (buf_curr_size >= 6U && buf_curr_size < this->msg_len + 6U - 1U) {
				this->msg_buf.push_back(b); // Message payload
			} else if (buf_curr_size == this->msg_len + 6U - 1U) {
				this->msg_buf.push_back(b); // Last byte!
				handleMessage();
				this->msg_buf.clear();
				this->msg_len = 0U;
			} else {
				this->msg_buf.clear();  // Unexpected byte, clear everything
				this->msg_len = 0U;
			}

			errno = 0;
		} else if (ret == 0) {
			// Socket has been closed, it should be handled by POLLHUP event in
			// the PollGroup
			// But if not, just in case, invalidate the FD.
			this->fd = -1;
			break;
		} else {
			// Some other error happened
			break;
		}
	} while(errno == 0); // Could be EAGAIN on EWOULDBLOCK

	if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR &&
			errno != EBADF) {
		this->fd = -1;
		int err = errno;
		std::cout << "Fail reading from the socket: " << errno << std::endl;
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(err)),
				"Fail reading from the socket");
	}
}

void Connection::handleMessage()
{
	// Parse the payload
	auto msg = std::make_shared<ft::proto::Message>(this->msg_buf);

	// Compose a Request
	auto req = std::make_shared<ft::request::Request>(this->shared_from_this(),
			msg);

	// Handle the request over to the RequestBroker
	if (sm_request_broker) {
		sm_request_broker->queueRequest(req);
	}
}

void Connection::sendBuffer(const std::vector<uint8_t>& buf)
{
	// TODO: handle concurrent access
	if (send(this->fd, (const void*)buf.data(), (size_t)buf.size(), 0) < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
			throw std::system_error(
					std::make_error_code(static_cast<std::errc>(errno)),
					"Failed sending data through the socket");
		}
	}
}

void Connection::setRequestBroker(request::RequestBrokerPtr req_broker)
{
	Connection::sm_request_broker = req_broker;
}

} // netwrk
} // ft