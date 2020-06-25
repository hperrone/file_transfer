//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#include <stdexcept>

#include "protocol/ft_msg_fctry.hpp"

namespace ft { namespace proto {

MessagePtr MessageFactory::buildMsgOffer(uint16_t seq_number,
		const boost::uuids::uuid& client_uuid, const file::FilePtr file)
{
	auto msg = std::make_shared<Message>();
	msg->msg_type            = MSGTYPE_FILE_OFFER;
	msg->seq_number          = seq_number;
	msg->client_uuid         = client_uuid;
	msg->file_name           = file->path.filename();
	msg->offer.file_size     = file->size;
	msg->offer.file_n_chunks = file->getNumOfChunks();
	msg->offer.file_hash.resize(file->hash.size());
	std::copy(file->hash.begin(), file->hash.end(), msg->offer.file_hash.begin());
	return msg;
}

MessagePtr MessageFactory::buildMsgChunkReq(uint16_t seq_number,
		const boost::uuids::uuid& client_uuid, const file::FilePtr file,
		const uint16_t chunk_idx_first, const uint16_t chunk_idx_last)
{
	auto msg = std::make_shared<Message>();
	msg->msg_type                  = MSGTYPE_FILE_CHUNK_REQ;
	msg->seq_number                = seq_number;
	msg->client_uuid               = client_uuid;
	msg->file_name                 = file->path.filename();
	msg->chunk_req.chunk_idx_first = chunk_idx_first;
	msg->chunk_req.chunk_idx_last  = chunk_idx_last;

	return msg;
}

MessagePtr MessageFactory::buildMsgChunkData(uint16_t seq_number,
		const boost::uuids::uuid& client_uuid, const file::FilePtr file,
		const uint16_t chunk_idx)
{
	auto msg = std::make_shared<Message>();
	msg->msg_type       = MSGTYPE_FILE_CHUNK_DATA;
	msg->seq_number     = seq_number;
	msg->client_uuid    = client_uuid;
	msg->file_name      = file->path.filename();
	msg->chunk_data.idx = chunk_idx;
	auto fchunk         = file->getChunk(chunk_idx);
	if (!fchunk) {
		throw std::runtime_error("Invalid chunk index");
	} else {
		msg->chunk_data.data.resize(fchunk->data.size());
		std::copy(fchunk->data.begin(), fchunk->data.end(),
				msg->chunk_data.data.begin());
	}

	return msg;
}

MessagePtr MessageFactory::buildMsgComplete(uint16_t seq_number,
		const boost::uuids::uuid& client_uuid, const file::FilePtr file)
{
	MessagePtr msg = std::make_shared<Message>();
	msg->msg_type                  = MSGTYPE_FILE_COMPLETE;
	msg->seq_number                = seq_number;
	msg->client_uuid               = client_uuid;
	msg->file_name                 = file->path.filename();

	return msg;
}

} // proto
} // ft