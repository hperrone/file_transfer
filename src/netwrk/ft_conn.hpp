//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#ifndef FT_NETWRK_CONN_H
#define FT_NETWRK_CONN_H

#include "ft_utils.hpp"
#include "request/ft_req_brkr.hpp"
#include "loop/ft_pollable.hpp"


namespace ft { namespace netwrk {

FT_DECLARE_CLASS(Connection)

/// @brief Generic Socket Connection
///
/// Each Connection instance represents holds a socket instance. It can be
/// from a connection established from the client to the server, or, on the
/// server side, a socket returned by accept.
///
/// The Connection is partially aware of the transfer protocol, and is
/// responsible to parse the envelope's header and then delegate the remaining
/// parsing duties to the ft::proto::Message class.
///
/// Once the Message is parsed, the Connection creates a Request instance and
/// passes it forward to the RequestBroker.
///
/// The RequestHandler may use the method sendBuffer() in the Connection to
/// reply to the Request.
///
/// The Connection implements the Pollable interface, making it possible to be
/// added to a PollGroup.
class Connection : virtual public loop::Pollable,
		public virtual std::enable_shared_from_this<Connection> {
private:
	/// The RequestBroker instance to handover the Request handling
	static request::RequestBrokerPtr    sm_request_broker;

	/// Buffer to collect bytes until completing the message to be parsed
	std::vector<uint8_t>    msg_buf;
	size_t                  msg_len;
public:
	/// @brief Used to build Connections from sockets obtained from accept()
	Connection(int fd);

	/// @brief Used on the client side to connect to a server
	Connection(const std::string& host, const uint16_t port);

	virtual ~Connection();

	/// @brief Handles events notified from the PollGroup
	///
	/// This is part of the Pollable interface
	virtual void handleEvent();

	virtual void sendBuffer(const std::vector<uint8_t>& buf);

	static void setRequestBroker(request::RequestBrokerPtr req_broker);

private:
	virtual void handleMessage();
};

} // netwrk
} // ft

#endif // FT_NETWRK_CONN_H