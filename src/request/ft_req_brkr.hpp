//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#ifndef FT_REQ_REQ_BRKR_H
#define FT_REQ_REQ_BRKR_H

#include <cstdint>
#include <vector>
#include <deque>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "request/ft_req_hndlr.hpp"
#include "ft_utils.hpp"

namespace ft { namespace request {

FT_DECLARE_CLASS(Request)
FT_DECLARE_CLASS(RequestBroker)
FT_DECLARE_CLASS(RequestBrokerWorker)

/// @brief Broke request among working threads
///
/// Provides the ability of splitting the request handling in multiple parallel
/// working threads.
///
/// The message reception and parsing is done in the main thread and then
/// handled over to the request handler.
///
/// Request handling implies accessing the local filesystem to read/write files
/// and calculating hashes. Doing this kind of blocking operations in the main
/// thread would block any other parallel connections and requests that may be
/// handled by the server or the client (it is possible for a client to
/// handle parallel uploads using the same connection).
///
/// The RequestBroker has a queue of Request. Connection instances after parsing
/// a received message, pushes the  Request to the end of the queue.
///
/// Each one of the RequestBrokerWorker instances runs on its own thread. They
/// simply peeks the first Request on the queue and invokes the associated
/// RequestHandler. Whenever the request queue is empty, each
/// RequestBrokerWorker blocks, waiting until some new Request is available to
/// be processed.  
class RequestBroker {
private:
	// TODO: Improve this by making these instance's members.
	static std::mutex              sm_req_brkr_mtx;
	static std::condition_variable sm_req_list_cv;
	static std::deque<RequestPtr>  sm_req_list;
	static bool                    sm_terminate;
	static RequestHandlerPtr       sm_req_hndlr;

	std::vector<RequestBrokerWorkerPtr> workers;
public:
	RequestBroker(RequestHandlerPtr req_hndlr, uint16_t n_workers);

public:
	virtual ~RequestBroker();

	/// @brief Push back a Request at the end of the requests queue.
	///
	/// This method must not block and return as soon as possible. 
	virtual void queueRequest(RequestPtr request);

private:
	void startWorkers(uint16_t n_threads);
	void stopWorkers();

friend class RequestBrokerWorker;
};

/// @brief Abstraction of a request handling thread
///
/// Each instance of RequestBrokerWorker has its own thread for handling request
/// Blocking or slow operations such as access to filesystem and hash
/// calculations can be done within the scope of a RequestBrokerWorker.
///
/// Each instance will block until a Request is added to the queue. When this
/// happens the first one managing to retrieve the Request from the queue,
/// handles it over to the associated RequestHandler, by invoking the method
/// RequestHandler::handleRequest().
///
/// Whenever the queue is empty, the RequestBrokerWorker instances will block,
/// waiting for a new request to be processed.
///  
class RequestBrokerWorker {
private:
	std::thread thread;

private:
	RequestBrokerWorker();

public:
	virtual ~RequestBrokerWorker();

private:
	/// Thread loop
	void run();

	/// Used to join the thread on RequestBroker's teardown
	void join();

friend class RequestBroker;

};

} // request
} // ft

#endif // FT_REQ_REQ_BRKR_H