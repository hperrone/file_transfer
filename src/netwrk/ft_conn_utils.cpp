//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

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
#include <fcntl.h>

#include "ft_conn_utils.hpp"

namespace ft { namespace netwrk {

void setup_socket_options(int fd)
{
	// Make socket non blocking
    int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) {
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(errno)),
				"Failed to get socket flags");
	}

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(errno)),
				"Failed to set socket flags to non block");
	}

	//int no_delay = 1;
	//setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &no_delay, sizeof(int));

	// Set the TCP keep alive setting for the connection.
	// The connection will be dropped after 10s of non-response to keep alive
	// probes.
	int flag_ = 1;
	int idle_tm = 1;     // time in seconds of idle connection before starting 
	                     // sending probes.
	int interval_tm = 1; // time in seconds between keep alive probes
	int keep_cnt = 10;   // number of probes to send before dropping connection

	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flag_, sizeof(int)) < 0) {
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(errno)),
				"Failed to set socket keep alive flag");
	}

	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle_tm, sizeof(int)) < 0) {
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(errno)),
				"Failed to set socket keep alive idle time");
	}
	
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval_tm, sizeof(int)) < 0) {
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(errno)),
				"Failed to set socket keep alive interval");
	}

	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &keep_cnt, sizeof(int)) < 0) {
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(errno)),
				"Failed to set socket keep alive counter");
	}
}

} // netwrk 
} // ft
