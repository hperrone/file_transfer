//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <system_error>

// POSIX & LINUX headers
#include <poll.h>
#include <unistd.h>

// App specific headers
#include "ft_utils.hpp"
#include "loop/ft_poll_grp.hpp"

namespace ft { namespace loop {

PollGroup::PollGroup(uint16_t max_pollables)
: max_pollables(max_pollables)
{}

PollGroup::~PollGroup()
{}

void PollGroup::add(PollablePtr pollable)
{
	if (this->pollables.size() >= this->max_pollables) {
		throw std::runtime_error("Maximum pollable limit exceeded");
	}

	// Whenever a new Pollable is added, rebuild the chached_pollfd array
	this->pollables.push_back(pollable);
	this->cached_pollfd.resize(this->pollables.size());
	for (size_t i = 0; i < this->pollables.size(); i++) {
		this->cached_pollfd.at(i).fd = this->pollables.at(i)->getFD();
		this->cached_pollfd.at(i).events = POLLIN;
	}
}

void PollGroup::remove(PollablePtr pollable)
{
	auto it = std::find( this->pollables.begin(), this->pollables.end(),
			pollable);

	if (it != this->pollables.end()) {
		this->pollables.erase(it);

		// Whenever a Pollable is removed, rebuild the chached_pollfd array
		this->cached_pollfd.resize(this->pollables.size());
		for (size_t i = 0; i < this->pollables.size(); i++) {
			this->cached_pollfd.at(i).fd = this->pollables.at(i)->getFD();
			this->cached_pollfd.at(i).events = POLLIN;
		}
	}
}

bool PollGroup::pollAndHandle()
{
	// Block and wait until there is something to process
	int ret = poll(this->cached_pollfd.data(), this->cached_pollfd.size(), 500);
	if (ret < 0) {
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(errno)),
				"Poll failed");
	}

	if (ret > 0) {
		for (size_t i = 0; i < this->pollables.size(); i++) {
			if (this->cached_pollfd.at(i).revents == 0) {
				// No events available for this fd, move to the next
				continue;
			}

			if ((this->cached_pollfd.at(i).revents & POLLIN) != 0) {
				// Let the Pollable handle the available data on its own handler
				try {
					this->pollables.at(i)->handleEvent();
				} catch(std::exception& e) {
					std::cout << e.what() << std::endl;
				}
			}

			if ((this->cached_pollfd.at(i).revents & POLLERR) != 0 ||
					(this->cached_pollfd.at(i).revents & POLLHUP) != 0 ||
					this->pollables.at(i)->getFD() == -1) {
				// The connection is closed or not longer valid.
				// So, remove the Pollable from the PollGroup.
				this->remove(this->pollables.at(i));
				i--;
			}
		}
	}

	return true;
}

} // loop
} // ft