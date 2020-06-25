//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#ifndef FT_LOOP_POLL_GRP_H
#define FT_LOOP_POLL_GRP_H

#include <vector>

// POSIX & LINUX headers
#include <poll.h>

#include "ft_utils.hpp"
#include "loop/ft_pollable.hpp"

namespace ft { namespace loop {

FT_DECLARE_CLASS(PollGroup)

/// @brief Aggregates a set of Pollable instances on which POSIX poll() can be
/// performed
///
/// This is the core class of the application's main loop.
///
/// Provides an abstraction of POSIX poll() and the array of struct pollfd.
/// Holds a dynamic vector of Pollables instances, each one exposing a fd. On
/// each invocation to pollAndHanle(), it invokes poll on the fds and when it
/// returns, invokes Pollable::handleEvent() on each Pollable with available
/// events.
class PollGroup {
private:
	size_t                     max_pollables;
	std::vector<PollablePtr>   pollables;
	std::vector<struct pollfd> cached_pollfd;

public:
	PollGroup(uint16_t max_pollables);
	virtual ~PollGroup();

	void add(PollablePtr pollable);
	void remove(PollablePtr pollable);

	/// Invokes poll() on the fd of the Pollables in the group and then invokes
	/// Pollable::handleEvent() for each Pollables with pending events.
	bool pollAndHandle();
};

} // loop
} // ft

#endif // FT_LOOP_POLL_GRP_H