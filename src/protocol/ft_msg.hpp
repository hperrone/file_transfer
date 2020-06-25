//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#ifndef FT_PROTOCOL_MSG_H
#define FT_PROTOCOL_MSG_H

#include <string>
#include <vector>

#include <boost/uuid/uuid.hpp>

#include "ft_utils.hpp"

namespace ft { namespace proto {

FT_DECLARE_CLASS(Message)

class MessageBuilder;

// The protocol is being designed to be also compatible with UDP, where the
// maximum datagram size is 4096B.
static const uint32_t MAX_MSG_SIZE = 4096U;

// In this case, 128 bytes are being reserved for protocol head, and using all
// the remaining bytes for delivering file's chunks payloads.
static const size_t MAX_MSG_PAYLOAD_SIZE = MAX_MSG_SIZE - 128U;

// Using BLAKE2 for hashing, digest is 64 bytes long
static const size_t HASH_SIZE = 64U;

// Using BLAKE2 for hashing chunks, but only 1 half of the digest
static const size_t CHUNK_HASH_SIZE = 32U;

// MAGIC number used to tag the beginning of each message
static const uint32_t MAGIC = 0x87FE7700;

/// Message type, works also as a magic number
typedef enum {
	MSGTYPE_FILE_OFFER      = MAGIC | 0x01,
	MSGTYPE_FILE_CHUNK_REQ  = MAGIC | 0x02,
	MSGTYPE_FILE_CHUNK_DATA = MAGIC | 0x03,
	MSGTYPE_FILE_COMPLETE   = MAGIC | 0x04,
	MSGTYPE_MAX
} MessageType;

/// @brief Message of the protocol
///
/// Parses, serializes and holds the information of the messages that are used
/// in the protocol for transferring files.
class Message {
public:
	MessageType          msg_type;    ///! Message type
	uint16_t             msg_len;     ///! Message length (from seq_number)
	uint16_t             seq_number;  ///! Message sequence number
	boost::uuids::uuid   client_uuid; ///! Client UUID
	std::string          file_name;   ///! File name

	/// Fields in FILE OFFER messages
	struct {
		uint32_t             file_size;     ///! Total file size
		uint32_t             file_n_chunks; ///! Number of chunks
		std::vector<uint8_t> file_hash;     ///! File hash [FILE_HASH]
	} offer;

	/// Fields in FILE CHUNK REQUEST messages
	struct {
		uint32_t             chunk_idx_first; ///! Chunk index being requested
		uint32_t             chunk_idx_last;  ///! (Not used)
	} chunk_req;

	/// Fields in FILE CHUNK DATA messages
	struct {
		uint32_t             idx;  ///! Chunk index
		std::vector<uint8_t> data; ///! Chunk data [up to MAX_MSG_PAYLOAD_SIZE]
		std::vector<uint8_t> hash; ///! Chunk hash [CHUNK_HASH_SIZE] (Not used)
	} chunk_data;

public:
	Message() {}
	Message(const std::vector<uint8_t>& buf);

	virtual ~Message() {}

	void serialize(std::vector<uint8_t>& out);

	friend class MessageFactory;
};

} // proto
} // ft

#endif // FT_PROTOCOL_MSG_H