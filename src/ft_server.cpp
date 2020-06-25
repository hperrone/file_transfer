//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#include <iostream>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "file/ft_file.hpp"
#include "loop/ft_poll_grp.hpp"
#include "loop/ft_signal.hpp"
#include "netwrk/ft_conn_listener.hpp"
#include "netwrk/ft_conn.hpp"
#include "protocol/ft_msg_fctry.hpp"
#include "request/ft_req_hndlr.hpp"
#include "request/ft_req.hpp"

static const uint16_t MAX_CONNECTIONS        = 1024;

static const uint16_t MAX_REQ_BROKER_THREADS = 16;

static const uint16_t DEFAULT_PORT           = 4444;

static const std::filesystem::path SERVER_BASE_PATH("/in");

FT_DECLARE_CLASS(ServerRequestHandler)

/// @brief RequestHandler specialization for server behavior
///
/// Implements the specific behavior of the server, waiting file offers and
/// requesting file chunks until the file is completed.
///
/// It is expected to be instantiated during the process initialization and
/// injected as a dependency into the RequestBroker.
///
/// It can be considered as an usage of inversion of control or strategy design
/// patterns since, when injected into the RequestBroker, it controls its
/// behavior.
class ServerRequestHandler : public virtual ft::request::RequestHandler {

public:
	ServerRequestHandler()
	: RequestHandler() {}

	virtual ~ServerRequestHandler() {}

	/// @brief Handles the requests dispatched from the RequestBroker
	virtual void handleRequest(ft::request::RequestPtr req);
};


int main(int argc, char* argv[]) {
	std::cout << "FT SERVER | Starting..." << std::endl;

	// -- Initialize and configure all the server components  -- //

	// Set the server output directory
	ft::file::File::setLocalPathPrefix(SERVER_BASE_PATH);

	// Instantiate the server components

	// The PollGroup handles 2 Pollables (the Connection and the SignalHandler)
	// plus the maximum number of supported connections.
	auto poll_group = std::make_shared<ft::loop::PollGroup>(MAX_CONNECTIONS + 2U);

	// SignalHandler to monitor SIGTERM, SIGQUIT, etc.
	auto signals    = std::make_shared<ft::loop::SignalHandler>();

	// Server socket connection listener
	auto server     = std::make_shared<ft::netwrk::ConnectionListener>(
			poll_group, DEFAULT_PORT, MAX_CONNECTIONS);

	// The ServerRequestHandler to control the server behavior
	auto server_req_handler = std::make_shared<ServerRequestHandler>();

	// The RequestBroker, using the ServerRequestHandler as flow control and
	// MAX_REQ_BROKER_THREADS working threads
	auto req_broker = std::make_shared<ft::request::RequestBroker>(
			server_req_handler, MAX_REQ_BROKER_THREADS);

	// -- Link the components -- //

	// Link the Connection to use the RequestBroker instance
	ft::netwrk::Connection::setRequestBroker(req_broker);

	// Add ConnectionListener and SignalHanlder instance to the PollGroup
	poll_group->add(server);
	poll_group->add(signals);

	std::cout << "FT SERVER | INIT COMPLETED" << std::endl;

	// -- Main Loop -- //

	// Loop handling events until termination is signaled
	while(poll_group->pollAndHandle() && !signals->receivedTermSignal());

	std::cout << "FT SERVER | Terminating..." << std::endl;
}



void ServerRequestHandler::handleRequest(ft::request::RequestPtr req)
{
	auto conn = req->getConnection();
	auto msg = req->getMessage();

	std::filesystem::path file_path = to_string(msg->client_uuid);
	file_path /= msg->file_name;

	ft::proto::MessagePtr response;

	switch(msg->msg_type) {
	case ft::proto::MSGTYPE_FILE_OFFER:
	{
		auto file = ft::file::File::makeRemoteFile(file_path,
				msg->offer.file_hash, msg->offer.file_size);
		if (file && file->isComplete()) {
			std::cout << "FT SERVER | File already transferred: " <<
					file_path.filename() << std::endl;

			response = ft::proto::MessageFactory::buildMsgComplete(
					msg->seq_number, msg->client_uuid, file);
		} else if (file) {
			size_t req_chunk_idx = file->getNextMissingChunk();
			response = ft::proto::MessageFactory::buildMsgChunkReq(
					msg->seq_number + 1, msg->client_uuid, file, req_chunk_idx,
					UINT16_MAX);

			// Reduce the log frequency to speed up the transfer
			if (file->getNumOfChunks() > 100 && (req_chunk_idx % 10) == 0) {
				std::cout << "FT SERVER | Request chunk: CID:"
					<< boost::uuids::to_string(msg->client_uuid)
					<< " - " << file_path.filename()
					<< "[" << req_chunk_idx << "]" << std::endl;
			}
		}
	}
	break;

	case ft::proto::MSGTYPE_FILE_CHUNK_DATA:
	{
		auto file = ft::file::File::makeRemoteFile(file_path);
		if (file) {
			auto fchunk = std::make_shared<ft::file::FileChunk>(file,
							msg->chunk_data.idx, msg->chunk_data.data,
							msg->chunk_data.hash);
			file->saveChunk(fchunk);

			if (file->isComplete()) {
				std::cout << "FT SERVER | File transferred: " <<
					file_path.filename() << std::endl;

				response = ft::proto::MessageFactory::buildMsgComplete(
						msg->seq_number, msg->client_uuid, file);
			} else {
				size_t req_chunk_idx = file->getNextMissingChunk();
				response = ft::proto::MessageFactory::buildMsgChunkReq(
						msg->seq_number + 1, msg->client_uuid, file,
						req_chunk_idx, UINT16_MAX);

				// Reduce the log frequency to speed up the transfer
				if (file->getNumOfChunks() > 100 && (req_chunk_idx % 10) == 0) {
					std::cout << "FT SERVER | Request chunk: CID:"
						<< boost::uuids::to_string(msg->client_uuid)
						<< " - " << file_path.filename()
						<< "[" << req_chunk_idx << "]"
						<< std::endl;
				}
			}
		}
	}
	break;

	default:
		break;  // Just ignore unsupported messages
	}

	if (response && conn) {
		std::vector<uint8_t> buf;
		response->serialize(buf);
		conn->sendBuffer(buf);
	}
}
