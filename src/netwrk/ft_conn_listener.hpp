//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#ifndef FT_NETWRK_CONN_LISTENER_H
#define FT_NETWRK_CONN_LISTENER_H

#include "ft_utils.hpp"
#include "loop/ft_pollable.hpp"
#include "loop/ft_poll_grp.hpp"

namespace ft { namespace netwrk {

FT_DECLARE_CLASS(ConnectionListener)

/// @brief Server Socket
///
/// Provides abstraction of a server socket listening for connections.
///
/// Whenever a socket connection is received, creates an instance of the
/// Connection object and adds it to the PollGroup instance received on its
/// constructor. Usually, it is the same PollGroup instance the
/// ConnectionListener is added to.
///
/// The Connection implements the Pollable interface, making it possible to be
/// added to a PollGroup.
class ConnectionListener : virtual public loop::Pollable {
private:
	loop::PollGroupPtr poll_group;
	int                listen_port;
	uint16_t           max_conn;
public:
	ConnectionListener(loop::PollGroupPtr poll_group, int listen_port,
			uint16_t max_conn);

	virtual ~ConnectionListener();

	/// @brief Handles events notified from the PollGroup it is added to
	///
	/// This is part of the Pollable interface
	virtual void handleEvent();
};

} // netwrk
} // ft

#endif // FT_NETWRK_CONN_LISTENER_H