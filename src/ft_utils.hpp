//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#ifndef FT_UTILS_H
#define FT_UTILS_H

#include <filesystem>
#include <memory>

#include <boost/uuid/uuid.hpp>

namespace ft {

//! Macro for declaring class and its shared and weak pointer types
#define FT_DECLARE_CLASS(CLASS_X)                   \
	class CLASS_X;                                  \
	typedef std::shared_ptr<CLASS_X> CLASS_X##Ptr;  \
	typedef std::weak_ptr<CLASS_X> CLASS_X##WPtr;

boost::uuids::uuid getClientUUID(const std::filesystem::path& uuid_file);

} // ft
#endif // FT_UTILS_H