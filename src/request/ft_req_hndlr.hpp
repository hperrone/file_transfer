//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#ifndef FT_REQ_REQHNDLR_H
#define FT_REQ_REQHNDLR_H

#include <cstdint>
#include <vector>
#include <deque>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "ft_utils.hpp"

namespace ft { namespace request {

FT_DECLARE_CLASS(Request)
FT_DECLARE_CLASS(RequestHandler)

/// @brief Abstract class for handling request
///
/// This abstract class defines the interface to be implemented by the handler
/// to be injected into the RequestBroker to handle the request.
///
/// The RequestBroker and the Connection are agnostic of the role of the process
/// (client or server). Whenever a Request arrives, the RequestBroker handles it
/// over to the RequestHandler to be processed.
class RequestHandler {

protected:
	RequestHandler() {}

public:
	virtual ~RequestHandler() {}

	/// @brief Invoked by the RequestBroker to handle a request
	virtual void handleRequest(RequestPtr request) = 0;
};

} // request
} // ft

#endif // FT_REQ_REQHNDLR_H