//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#ifndef FT_NETWRK_CONN_UTILS_H
#define FT_NETWRK_CONN_UTILS_H

namespace ft { namespace netwrk {

/// @brief Setup socket options
///
/// Sets the socket to be NON BLOCKING and setups the TCP keep alive options.
void setup_socket_options(int fd);

} // netwrk
} // ft

#endif // FT_NETWRK_CONN_UTILS_H