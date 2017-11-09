/////////////////
// spqueue.cpp //
/////////////////

#include "zuse.hpp"

#include <iostream>
#include <string>
#include <queue>
#include <cstdlib>

#define MAX_WORKERS		100

const char* backend_addr = "tcp://*:5556";
const char* frontend_addr = "tcp://*:5555";

int main(int argc, char* argv[]) {
	queue<string> worker_queue;
	
	zuse::context_t context;
	
	zuse::state_t bad_worker("received bad message from worker");
	zuse::state_t no_workers("no workers available");
	zuse::state_t workers_ready("we have workers available");
	
	bad_worker.add_substate(no_worker);
	bad_worker.add_substate(workers_ready);
	
	no_workers.add_condition([&](zuse::event::context_t& c) {
		return worker_queue.size() == 0;
	});
	
	zuse::message_t bad_msg("invalid");
	zuse::message_t ready("worker ready", {
		{"id", zuse::message_t::any}, 
		{"envelope", zuse::message_t::envelope},
		{"ready", "READY"}
	});
	zuse::message_t reply("worker reply", {
		{"id", zuse::message_t::any}, 
		{"envelope", zuse::message_t::envelope},
		{"msg", zuse::message_t::any}
	});
	zuse::message_t request("client request");
	
	no_workers.on_recv(ready, [&](zuse::event::context_t& c) {
		worker_queue.push(c.frame(0));
	}).next_state(workers_ready);
	
	workers_ready.on_recv(ready, [&](zuse::event::context_t& c) {
		worker_queue.push(c.frame(0));
	});
	
	workers_ready.on_recv(request, [&](zuse::event::context_t& c) {
		
		
	});
	
	workers_ready.on_recv(reply, [&](zuse::event::context_t& c) {
		assert(worker_queue.size() < MAX_WORKERS);
		
		worker_queue.push(c.frame(0));
		c.send(c.frames().begin() + 2, c.frames().end());
	});
	
	
	bad_worker.on_recv(bad_msg, [&](zuse::event::context_t& c) {
		cerr << "received bad message from worker:" << endl;
		for (f : c.frames())
			cerr << "\t" << f << endl;
		throw std::exception("received bad message from worker");
	});
	
	
	
	
	
	auto backend = context.bind(backend_addr, zuse::bind_type::router);
	auto frontend = context.bind(frontend_addr, zuse::bind_type::router);
	
	no_workers.add_receiver(backend);
	workers_ready.add_receivers({backend, frontend});
	workers_ready.add_sender(frontend);
	
	context.poll(no_workers);
	
	
	
	
}











