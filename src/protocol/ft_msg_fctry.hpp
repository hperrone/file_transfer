//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#ifndef FT_PROTOCOL_MSG_FACTORY_H
#define FT_PROTOCOL_MSG_FACTORY_H

#include <boost/uuid/uuid.hpp>

#include "protocol/ft_msg.hpp"
#include "file/ft_file.hpp"
#include "ft_utils.hpp"

namespace ft { namespace proto {

/// @brief Provide convenience methods for instantiating and filling Messages
///
class MessageFactory {
public:
	static MessagePtr buildMsgOffer(uint16_t seq_number,
			const boost::uuids::uuid& client_uuid, const file::FilePtr file);

	static MessagePtr buildMsgChunkReq(uint16_t seq_number,
			const boost::uuids::uuid& client_uuid, const file::FilePtr file,
			const uint16_t chunk_idx_first, const uint16_t chunk_idx_last);

	static MessagePtr buildMsgChunkData(uint16_t seq_number,
			const boost::uuids::uuid& client_uuid, const file::FilePtr file,
			const uint16_t chunk_idx);

	static MessagePtr buildMsgComplete(uint16_t seq_number,
			const boost::uuids::uuid& client_uuid, const file::FilePtr file);

};

} // proto
} // ft

#endif // FT_PROTOCOL_MSG_FACTORY_H