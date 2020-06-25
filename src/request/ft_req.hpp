//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#ifndef FT_REQ_REQ_H
#define FT_REQ_REQ_H

#include <deque>

#include "protocol/ft_msg.hpp"
#include "netwrk/ft_conn.hpp"
#include "ft_utils.hpp"

namespace ft { namespace request {

FT_DECLARE_CLASS(Request)

/// @brief Simple data object representing a request received from a Connection
///
/// The request aggregates the Message received and the Connection instance from
/// on which such message have been received. If a response is due to be sent,
/// then such response must be returned from such Connection.
class Request {
private:
	// A weak pointer is used for the connection since it may be destroyed
	// before handling the message.
	netwrk::ConnectionWPtr connection;
	proto::MessagePtr      message;
public:
	Request(netwrk::ConnectionPtr connection, proto::MessagePtr message)
	: connection(connection)
	, message(message)
	{}
public:
	virtual ~Request() {}

	virtual netwrk::ConnectionPtr getConnection() const {
		return connection.lock();
	}

	virtual proto::MessagePtr getMessage() const { return message; }
private:
};

} // request
} // ft

#endif // FT_REQ_REQ_H