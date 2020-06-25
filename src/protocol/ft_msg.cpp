//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <string>
#include <stdexcept>
#include <vector>

#include "protocol/ft_msg.hpp"

namespace ft { namespace proto {

// -- Helper functions for writing and parsing raw messages -- //
// -- Every numeric type is in network byte order           -- //

static uint32_t getU32(std::vector<uint8_t>::const_iterator& it);
static uint16_t getU16(std::vector<uint8_t>::const_iterator& it);
static void putU32(std::back_insert_iterator<std::vector<uint8_t>>& it,
		uint32_t val);
static void putU16(std::back_insert_iterator<std::vector<uint8_t>>& it,
		uint16_t val);

Message::Message(const std::vector<uint8_t>& buf)
{
	// -- Parses the raw message buffer and fills the class members -- //

	auto it           = buf.begin();
	this->msg_type    = (MessageType)getU32(it);
	this->msg_len     = getU16(it);
	this->seq_number  = getU16(it);

	// Client UUID
	for (size_t i = 0U; i < this->client_uuid.size(); i++) {
		this->client_uuid.data[i] = *it; it++;
	}

	// File name
	int file_name_len = *it; it++;
	std::copy_n(it, file_name_len, std::back_inserter(this->file_name));
	std::advance(it, file_name_len);

	// Parse variable part, depending on the message type
	uint16_t chunk_len;
	switch(this->msg_type) {
	case MSGTYPE_FILE_OFFER:
		this->offer.file_size     = getU32(it);
		this->offer.file_n_chunks = getU32(it);
		std::copy_n(it, HASH_SIZE, std::back_inserter(this->offer.file_hash));
		break;
	case MSGTYPE_FILE_CHUNK_REQ:
		this->chunk_req.chunk_idx_first = getU32(it);
		this->chunk_req.chunk_idx_last  = getU32(it);
		break;
	case MSGTYPE_FILE_CHUNK_DATA:
		this->chunk_data.idx  = getU32(it);
		chunk_len             = getU16(it);
		std::copy_n(it, chunk_len, std::back_inserter(this->chunk_data.data));
		this->chunk_data.hash.resize(CHUNK_HASH_SIZE);
		break;
	case MSGTYPE_FILE_COMPLETE:
		// no additional information in this king of message
		break;
	default:
		throw std::runtime_error("Not supported Msg Type");
	}
}

void Message::serialize(std::vector<uint8_t>& out)
{
	// First build the contents after the msg_length field
	std::vector<uint8_t> tmpbuf;
	auto tmpout = std::back_inserter(tmpbuf);

	putU16(tmpout, this->seq_number);

	std::copy(this->client_uuid.begin(), this->client_uuid.end(), tmpout);

	size_t filename_len = this->file_name.length() < UINT8_MAX ?
			this->file_name.length() : UINT8_MAX;
	*tmpout = (uint8_t)(filename_len & 0xff);
	std::copy_n(this->file_name.begin(), filename_len, tmpout);

	switch(this->msg_type) {
	case MSGTYPE_FILE_OFFER:
		putU32(tmpout, this->offer.file_size);
		putU32(tmpout, this->offer.file_n_chunks);
		std::copy_n(this->offer.file_hash.begin(), HASH_SIZE, tmpout);
		break;
	case MSGTYPE_FILE_CHUNK_REQ:
		putU32(tmpout, this->chunk_req.chunk_idx_first);
		putU32(tmpout, this->chunk_req.chunk_idx_last);
		break;
	case MSGTYPE_FILE_CHUNK_DATA:
		putU32(tmpout, this->chunk_data.idx);
		if (this->chunk_data.data.size() > MAX_MSG_PAYLOAD_SIZE ||
				this->chunk_data.data.size() == 0) {
			throw std::length_error("Invalid chunk length");
		}

		putU16(tmpout, (uint16_t)this->chunk_data.data.size());
		std::copy(this->chunk_data.data.begin(),
				this->chunk_data.data.end(), tmpout);
		break;
	case MSGTYPE_FILE_COMPLETE:
		break;
	default:
		throw std::invalid_argument("Invalid MessageType");
	}

	// Once the contents are complete, write the message with the envelope
	auto itout = std::back_inserter(out);
	putU32(itout, this->msg_type);
	putU16(itout, tmpbuf.size());
	std::copy(tmpbuf.begin(), tmpbuf.end(), itout);
}

static uint32_t getU32(std::vector<uint8_t>::const_iterator& it)
{
	uint32_t ret = 0;
	ret |= (((uint32_t)(*it)) << 24) & 0xff000000; it++;
	ret |= (((uint32_t)(*it)) << 16) & 0x00ff0000; it++;
	ret |= (((uint32_t)(*it)) <<  8) & 0x0000ff00; it++;
	ret |= (((uint32_t)(*it))      ) & 0x000000ff; it++;
	return ret;
}

static uint16_t getU16(std::vector<uint8_t>::const_iterator& it)
{
	uint16_t ret = 0;
	ret |= (((uint16_t)(*it)) <<  8) & 0xff00; it++;
	ret |= (((uint16_t)(*it))      ) & 0x00ff; it++;
	return ret;
}

static void putU32(std::back_insert_iterator<std::vector<uint8_t>>& it,
		uint32_t val)
{
	*it = (uint8_t)((val >> 24) & 0xff); it++;
	*it = (uint8_t)((val >> 16) & 0xff); it++;
	*it = (uint8_t)((val >>  8) & 0xff); it++;
	*it = (uint8_t)((val      ) & 0xff); it++;
}

static void putU16(std::back_insert_iterator<std::vector<uint8_t>>& it,
		uint16_t val)
{
	*it = (uint8_t)((val >>  8) & 0xff); it++;
	*it = (uint8_t)((val      ) & 0xff); it++;
}

} // proto
} // ft