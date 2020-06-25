//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <system_error>

// POSIX & LINUX headers
#include <stdlib.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <unistd.h>

// App specific headers
#include "ft_utils.hpp"
#include "loop/ft_signal.hpp"

namespace ft { namespace loop {

SignalHandler::SignalHandler()
: Pollable(-1)
, terminate_signal(false)
{
	sigset_t sig_mask;

	// Only handle termination signals
	sigemptyset(&sig_mask);
	sigaddset(&sig_mask, SIGINT);  // "Ctrl-C"
	sigaddset(&sig_mask, SIGQUIT); // "Ctrl-\"
	sigaddset(&sig_mask, SIGTERM); // kill
	sigaddset(&sig_mask, SIGTSTP); // "Ctrl-Z"
	sigaddset(&sig_mask, SIGHUP);  // Terminal closed

	if (sigprocmask(SIG_BLOCK, &sig_mask, NULL) < 0) {
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(errno)),
				"Failed to set SIG_BLOCK");
	}

	int tmpfd;
	tmpfd = signalfd(-1, &sig_mask, SFD_NONBLOCK);
	if (tmpfd < 0) {
		throw std::system_error(
				std::make_error_code(static_cast<std::errc>(errno)),
				"Failed to create signal fd");
	}

	this->fd = tmpfd;
}

SignalHandler::~SignalHandler()
{
	if (this->fd >= 0) {
		(void)close(this->fd);
	}
}

void SignalHandler::handleEvent()
{
	// This is part of the Pollable interface and called from the PollGroup
	// whenever an event is notified for the underlying fd.

	struct signalfd_siginfo siginfo;
	while(read(this->fd, &siginfo, sizeof(siginfo)) > 0) {
		std::cout << "Signal received: " << siginfo.ssi_signo << std::endl;
	}
	// If any termination signal received, then update the termination flag
	terminate_signal = true;
}

} // loop
} // ft