//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <system_error>

// POSIX & LINUX headers
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

// App specific headers
#include "ft_utils.hpp"
#include "netwrk/ft_conn.hpp"
#include "netwrk/ft_conn_utils.hpp"
#include "netwrk/ft_conn_listener.hpp"

namespace ft { namespace netwrk {

ConnectionListener::ConnectionListener(loop::PollGroupPtr poll_group,
		int listen_port, uint16_t max_conn)
: Pollable(-1)
, poll_group(poll_group)
, listen_port(listen_port)
, max_conn(max_conn)
{
	// -- Open and setup a server Socket for listening on the given port -- //

	// Open the TCP socket
	int tmpfd = socket(AF_INET, SOCK_STREAM, 0);
	if (tmpfd < 0) {
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(errno)),
				"Failed to open listener socket");
	}

	// Set the reuse address and port options
	int op = 1;
	if (setsockopt(tmpfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &op,
			sizeof(op)) < 0) {
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(errno)),
				"Failed to set socket option");
	}

	// Set up additional socket's options common client and server sockets
	setup_socket_options(tmpfd);

	// Bind to the given port
	struct sockaddr_in listen_addr = {
		.sin_family = AF_INET,
		.sin_addr = { .s_addr = INADDR_ANY }
	};
	listen_addr.sin_port = htons(this->listen_port);

	if (bind(tmpfd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(errno)),
				"Failed to bind socket to listen address/port");
	}

	// Start listening for connections
	if (listen(tmpfd, this->max_conn) < 0) {
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(errno)),
				"Failed to start listening on address/port");
	}

	// Finally, update the Pollable::fd
	this->fd = tmpfd;
}

ConnectionListener::~ConnectionListener()
{
	if (this->fd >= 0) {
		(void)close(this->fd);
	}
}

void ConnectionListener::handleEvent()
{
	// This is part of the Pollable interface and called from the PollGroup
	// whenever an event is notified for the underlying fd.

	int conn_sock_fd;

	// Accept all the connections that may be required to this socket,
	// instantiate a Connection object, and add it to the PollGroup
	while((conn_sock_fd = accept(this->fd, NULL, NULL)) >= 0) {
		std::cout << "FT SERVER | new connection" << std::endl;
		ConnectionPtr conn = std::make_shared<Connection>(conn_sock_fd);
		this->poll_group->add(conn);
	}

	if (errno != EWOULDBLOCK && errno != EAGAIN)
	{
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(errno)),
				"Accept failed on socket listener");
	}
}

} // netwrk
} // ft