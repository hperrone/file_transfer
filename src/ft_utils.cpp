//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "ft_utils.hpp"

namespace ft {

boost::uuids::uuid getClientUUID(const std::filesystem::path& uuid_file)
{
	boost::uuids::uuid ret;
	if (std::filesystem::exists(uuid_file) &&
			!std::filesystem::is_regular_file(uuid_file)) {
		throw std::runtime_error(std::string("UIID file is not a regular file: "
				) + (std::string)uuid_file);
	}

	if (std::filesystem::exists(uuid_file)) {
		std::stringstream ss;
		std::ifstream is(uuid_file, std::ifstream::in);
		ss << is.rdbuf();
		is.close();
		ss >> ret;
	} else {
		std::filesystem::create_directories(uuid_file.parent_path());
		ret = boost::uuids::random_generator()();
		std::ofstream os(uuid_file, std::ifstream::out);
		os << to_string(ret);
		os.close();
	}

	return ret;
}

} // ft