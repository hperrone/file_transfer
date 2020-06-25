//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#ifndef FT_LOOP_POLLABLE_H
#define FT_LOOP_POLLABLE_H

#include <vector>

#include "ft_utils.hpp"

namespace ft { namespace loop {

FT_DECLARE_CLASS(Pollable)

/// @brief Base class for elements that can be added to a PollGroup
///
/// Abstraction of an entity with a file descriptor that can be used with
/// poll().
///
/// The Pollable can be added to a PollGroup so it can be polled at the same
/// time with other objects.
///
/// Whenever an event is available in the fd, the handleEvent() method is
/// invoked.
class Pollable {
protected:
	int fd;

protected:
	Pollable(int fd) : fd(fd) {};

public:
	virtual ~Pollable() {};

	int getFD() const { return this->fd; }

	/// @brief Handle events available associated with the fd
	virtual void handleEvent() = 0;
};

} // loop
} // ft

#endif // FT_LOOP_POLLABLE_H