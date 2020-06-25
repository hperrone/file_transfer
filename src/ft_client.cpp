//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#include <filesystem>
#include <iostream>
#include <string>
#include <map>
#include <unistd.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include "ft_utils.hpp"
#include "protocol/ft_msg_fctry.hpp"
#include "loop/ft_poll_grp.hpp"
#include "loop/ft_signal.hpp"
#include "netwrk/ft_conn.hpp"
#include "file/ft_file.hpp"
#include "request/ft_req_hndlr.hpp"
#include "request/ft_req.hpp"

static const std::filesystem::path CLIENT_UUID_FILE("/.ft_client/.uuid");

static const uint16_t DEFAULT_PORT = 4444;

static void show_usage(std::ostream& out, const char* app);

FT_DECLARE_CLASS(ClientRequestHandler)


/// @brief RequestHandler specialization for client behavior
///
/// Implements the specific behavior of the client, offering file and delivering
/// the file chunks requested by the server
///
/// It is expected to be instantiated during the process initialization and
/// injected as a dependency into the RequestBroker.
///
/// It can be considered as an usage of inversion of control or strategy design
/// patterns since, when injected into the RequestBroker, it controls its
/// behavior.
class ClientRequestHandler : public virtual ft::request::RequestHandler {
private:
	const boost::uuids::uuid                 client_uuid;
	std::map<std::string, ft::file::FilePtr> client_files;

public:
	ClientRequestHandler(const boost::uuids::uuid client_uuid)
	: RequestHandler()
	, client_uuid(client_uuid)
	{}

	virtual ~ClientRequestHandler() {}

	/// @brief Handles the requests dispatched from the RequestBroker
	virtual void handleRequest(ft::request::RequestPtr request);

	/// @brief Action to offer files to be uploaded to the server.
	virtual void offer(ft::netwrk::ConnectionPtr conn, ft::file::FilePtr file);

	/// @brief Check if all the uploads are completed
	bool uploadsCompleted() const;
};


int main(int argc, char* argv[]) {

	// -- Default parameters -- //

	std::string           host("localhost");
	uint16_t              port        = DEFAULT_PORT;
	boost::uuids::uuid    client_uuid = ft::getClientUUID(CLIENT_UUID_FILE);
	std::filesystem::path file; // Mandatory to be provided in command line


	// -- Parse command line arguments and update parameters -- //

	int opt;
	while ((opt = getopt(argc, argv, "hd:p:u:")) != -1) {
		switch (opt) {
		case 'h': show_usage(std::cout, argv[0]); exit(0); break;
		case 'd': host = optarg;                           break;
		case 'p': port = atoi(optarg);                     break;
		case 'u':
			client_uuid = boost::lexical_cast<boost::uuids::uuid>(optarg);
			break;
		default:  show_usage(std::cerr, argv[0]); exit(1); break;
		}
	}

	if (optind >= argc) {
		std::cerr << "ERROR: File is missing" << std::endl;
		show_usage(std::cerr, argv[0]);
		exit(1);
	}

	file = std::filesystem::path(argv[optind]);
	if (!std::filesystem::exists(file)) {
		std::cerr << "ERROR: File '" << file << "' does not exist." << std::endl;
		show_usage(std::cerr, argv[0]);
		exit(1);
	}

	std::cout << "FT CLIENT | Starting..." << std::endl;
	std::cout << "FT CLIENT |   UUID:   " << to_string(client_uuid) << std::endl;
	std::cout << "FT CLIENT |   SERVER: " << host << ":" << port << std::endl;
	std::cout << "FT CLIENT |   FILE:   " << file << std::endl;

	// -- Initialize all the components handling the client -- //

	// The PollGroup just handles 2 Pollables: the Connection and the
	// SignalHandler
	auto poll_group = std::make_shared<ft::loop::PollGroup>(2U);

	// SignalHandler to monitor SIGTERM, SIGQUIT, etc.
	auto signals    = std::make_shared<ft::loop::SignalHandler>();

	// Client just have a single Connection instance
	auto conn       = std::make_shared<ft::netwrk::Connection>(host, port);

	// The ClientRequestHandler to control the client behavior
	auto client_req_hndlr = std::make_shared<ClientRequestHandler>(client_uuid);

	// The file to upload
	auto localFile  = ft::file::File::makeLocalFile(file);

	// The RequestBroker, using the ClientRequestHandler as flow control and 1
	// single working thread
	auto req_broker = std::make_shared<ft::request::RequestBroker>(
			client_req_hndlr, 1U);


	// -- Link the components -- //

	// Link the Connection to use the RequestBroker instance
	ft::netwrk::Connection::setRequestBroker(req_broker);

	// Add Connection and SignalHanlder instance to the PollGroup
	poll_group->add(signals);
	poll_group->add(conn);

	std::cout << "FT CLIENT | INIT COMPLETED" << std::endl;


	// -- Make the ClientRequestHandler to offer the file -- //


	// Start the interaction by offering a file to the server
	client_req_hndlr->offer(conn, localFile);
	// Optionally, more files can be offered

	// -- Main Loop -- //


	// Loop handling events until termination is signaled and all uploads are
	// completed
	while(!client_req_hndlr->uploadsCompleted() &&
			poll_group->pollAndHandle() &&
			!signals->receivedTermSignal());

	std::cout << "FT CLIENT | Terminating..." << std::endl;
}


static void show_usage(std::ostream& out, const char* app)
{
	std::filesystem::path app_name = app;

	out
		<< "Usage: " << app_name.filename().generic_string()
					 << "<option(s)> FILE" << std::endl
		<< "Options:" << std::endl
		<< "\t-h\t\tShow this help message" << std::endl
		<< "\t-d SERVER\t\tDestination server" << std::endl
		<< "\t-p PORT\t\tDestination port" << std::endl
		<< "\t-u UUID\t\tClient UUID" << std::endl;
}


void ClientRequestHandler::handleRequest(ft::request::RequestPtr req)
{
	auto conn = req->getConnection();
	auto msg = req->getMessage();

	// If the message is not for the current client, discard it.
	if (msg->client_uuid != this->client_uuid) {
		std::cout << "Invalid client id: " << to_string(msg->client_uuid) <<
				std::endl;
		return;
	}

	// Check if the file in the message is one being offered by the client
	auto it = this->client_files.find(msg->file_name);
	if (it == this->client_files.end()) {
		std::cout << "Not offered file: " << msg->file_name << std::endl;
		return;
	}

	auto file = it->second;

	ft::proto::MessagePtr response;

	switch(msg->msg_type) {
	case ft::proto::MSGTYPE_FILE_CHUNK_REQ:
		// Send the requested file chunk
		response = ft::proto::MessageFactory::buildMsgChunkData(msg->seq_number,
				this ->client_uuid, file, msg->chunk_req.chunk_idx_first);
		break;
	case ft::proto::MSGTYPE_FILE_COMPLETE:
		// The server already have the file remove from the offered file lists
		this->client_files.erase(msg->file_name);
	default: 
		break; // Just ignore unsupported messages
	}

	// Send the response to the server
	if (response && conn) {
		std::vector<uint8_t> buf;
		response->serialize(buf);
		conn->sendBuffer(buf);
	}
}

void ClientRequestHandler::offer(ft::netwrk::ConnectionPtr conn,
		ft::file::FilePtr file)
{
	// Add the file to the map. Instantiation of this object is expensive due to
	// hash calculation.
	this->client_files.insert({file->path.filename().generic_string(), file});

	// Prepare the File Offer message and serialize to buf
	std::vector<uint8_t> buf;
	auto msg = ft::proto::MessageFactory::buildMsgOffer(1, this->client_uuid,
			file);
	msg->serialize(buf);

	// Finally use the connection to send the serialized message
	conn->sendBuffer(buf);
}

bool ClientRequestHandler::uploadsCompleted() const {
	// True if all the files have been removed from the map
	return this->client_files.empty();
}