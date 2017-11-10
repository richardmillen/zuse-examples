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
	zuse::context_t context;
	queue<string> worker_queue;
	
	zuse::state_t bad_worker("received bad message from worker");
	zuse::state_t no_workers("no workers available");
	zuse::state_t ready("ready to handle requests (workers available)");
	
	bad_worker.add_substate(no_worker);
	bad_worker.add_substate(ready);
	
	zuse::message_t bad_msg("invalid");
	zuse::message_t worker_ready("worker ready", {
		{"id", zuse::message_t::any}, 
		{"envelope", zuse::message_t::envelope},
		{"ready", "READY"}
	});
	zuse::message_t worker_reply("worker reply", {
		{"id", zuse::message_t::any}, 
		{"envelope", zuse::message_t::envelope},
		{"msg-frames", zuse::message_t::any_frames}
	});
	zuse::message_t client_request("client request");
	zuse::message_t fwd_request("request forwarded from client to worker", {
		{"id", zuse::message_t::any},
		{"envelope", zuse::message_t::envelope}
		{"msg-frames", zuse::message_t::any_frames}
	});
	
	auto backend = context.bind(backend_addr, zuse::bind_type::router);
	auto frontend = context.bind(frontend_addr, zuse::bind_type::router);
	
	no_workers.add_condition([&](zuse::event::context_t& c) {
		return worker_queue.size() == 0;
	});
	
	backend.on_recv(worker_ready).raises(no_workers, [](zuse::context_t& c) {
		
	});
	
	no_workers.on_recv(worker_ready, backend, [&](zuse::event::context_t& c) {
		worker_queue.push(c.frame(0));
	}).next_state(ready);
	
	ready.on_recv(worker_ready, backend, [&](zuse::event::context_t& c) {
		worker_queue.push(c.frame(0));
	});
	
	ready.on_recv(client_request, [&](zuse::event::context_t& c) {
		auto worker_id = worker_queue.front();
		worker_queue.pop();
		
		backend.send_more(worker_id);
		backend.send_more(string());
		backend.send(c.frames());
	}).next_state(no_workers);
	
	ready.on_recv(worker_reply, [&](zuse::event::context_t& c) {
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
	
	
	
	
	
	context.poll(no_workers);
	
	
	
	
}




