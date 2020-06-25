//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#include <cstdint>
#include <iostream>
#include <thread>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "netwrk/ft_conn.hpp"
#include "protocol/ft_msg_fctry.hpp"
#include "request/ft_req_brkr.hpp"

namespace ft { namespace request {

// Two critical regions are protected with a single mutex: 
//  - termination flag
//  - Request queue 
// Also the conditional variable sm_req_list_cv is used with this mutex
std::mutex              RequestBroker::sm_req_brkr_mtx;

// Conditional variable to signal the workers when new Request are added to the
// queue or the termination flag value is changed.
std::condition_variable RequestBroker::sm_req_list_cv;

// Requests queue
std::deque<RequestPtr>  RequestBroker::sm_req_list; 

// Termination flag
bool                    RequestBroker::sm_terminate = false;

// Instance of the RequestHandler designated to handle the requests
RequestHandlerPtr       RequestBroker::sm_req_hndlr;

RequestBroker::RequestBroker(RequestHandlerPtr req_hndlr, uint16_t n_workers)
{
	sm_req_hndlr = req_hndlr;
	startWorkers(n_workers);
}

RequestBroker::~RequestBroker()
{
	stopWorkers();
}

void RequestBroker::startWorkers(uint16_t n_workers)
{ 
	// START TERMINATION FLAG CRITICAL REGION
	std::unique_lock<std::mutex> lock(sm_req_brkr_mtx);
	sm_terminate = false;

	for (uint16_t i = 0; i < n_workers; i++) {
		// Instantiate the given number of RequestBrokerWorkers.
		RequestBrokerWorkerPtr reqHndlr(new RequestBrokerWorker());

		// Start the thread using RequestBrokerWorker::run() as entry point
		std::thread new_thread(&RequestBrokerWorker::run, reqHndlr);
		reqHndlr->thread = std::move(new_thread);

		this->workers.push_back(reqHndlr);
	}
	// END TERMINATION FLAG CRITICAL REGION
}

void RequestBroker::stopWorkers()
{
	{ // START TERMINATION FLAG CRITICAL REGION
		// Signal all the threads to terminate
		std::unique_lock<std::mutex> lock(sm_req_brkr_mtx);
		sm_terminate = true;
		sm_req_list_cv.notify_all();
	} // END TERMINATION FLAG CRITICAL REGION

	// Join each thread until all are terminated
	for (auto it = this->workers.begin(); it != this->workers.end(); ++it) {
		try {
			(*it)->join();
		} catch (...) {

		}
	}

	this->workers.clear();
}

void RequestBroker::queueRequest(RequestPtr request)
{
	// START QUEUE CRITICAL REGION
	const std::lock_guard<std::mutex> lock(sm_req_brkr_mtx);

	// Add the request to the end of the queue
	sm_req_list.push_back(request);

	// Notify any worker that may be waiting
	sm_req_list_cv.notify_all();

	// END QUEUE CRITICAL REGION
}

RequestBrokerWorker::RequestBrokerWorker()
{}

RequestBrokerWorker::~RequestBrokerWorker()
{}

void RequestBrokerWorker::run()
{
	// RequestBrokerWorker's thread entry point

	//std::cout << "[ReqHndlrSrv - " << std::this_thread::get_id() << "] - START"
	//		<< std::endl;  

	// Loop until the terminate flag is set - no need to synchronize this
	while(!RequestBroker::sm_terminate) {
		RequestPtr req;
		
		{	// START QUEUE CRITICAL REGION  
			std::unique_lock<std::mutex> lock( RequestBroker::sm_req_brkr_mtx);

			// Block while the queue is empty and the termination flag is not
			// set
			while(RequestBroker::sm_req_list.empty() &&
					!RequestBroker::sm_terminate) {
				RequestBroker::sm_req_list_cv.wait(lock);
			}

			// In the termination flag is set leave the loop
			if (RequestBroker::sm_terminate) {
				continue;
			}
			
			// Retrieve the first Request from the queue
			req = RequestBroker::sm_req_list.front();
			RequestBroker::sm_req_list.pop_front();
		} // END QUEUE CRITICAL REGION

		//std::cout << "[ReqHndlrSrv - " << std::this_thread::get_id() << "] - "
		//	"Hanlding MSG" << std::endl;
		
		// Handle the Request to the RequestHandler (it may take time to process)
		try {
			RequestBroker::sm_req_hndlr->handleRequest(req);
		} catch(std::exception& e) {
			std::cout << e.what() << std::endl;
		}
		
	}

	//std::cout << "[ReqHndlrSrv - " << std::this_thread::get_id() << "] - END"
	//	<< std::endl;
}

void RequestBrokerWorker::join()
{
	this->thread.join();
}

} // request 
} // ft