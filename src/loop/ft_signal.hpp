//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#ifndef FT_LOOP_SIGNAL_H
#define FT_LOOP_SIGNAL_H

#include "ft_utils.hpp"
#include "loop/ft_pollable.hpp"

namespace ft { namespace loop {

FT_DECLARE_CLASS(SignalHandler)

/// @brief Receives termination signals
///
/// Provides an abstraction of POSIX's signalfd to handle SIGINT, SIGTERM,
/// SIGQUIT, SIGTSTP and SIGHUP.
///
/// The SignalHandler implements the Pollable interface and it is intended to
/// be added to a PollGroup to monitor the occurrences of termination signals.
class SignalHandler : virtual public Pollable {
private:
    bool terminate_signal;
public:
	SignalHandler();
	virtual ~SignalHandler();

	/// @brief Handles events notified from the PollGroup
	///
	/// This is part of the Pollable interface
	virtual void handleEvent();

	/// Returns true if any of the monitored signals have been received
    virtual bool receivedTermSignal() const { return this->terminate_signal; }
};

} // loop
} // ft

#endif // FT_LOOP_SIGNAL_H