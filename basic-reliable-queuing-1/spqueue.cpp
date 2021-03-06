/////////////////
// spqueue.cpp //
/////////////////

#include "zuse.hpp"

#include <iostream>
#include <string>
#include <queue>

using namespace std;

int main(int argc, char* argv[]) {
	zuse::context queue;
	
	zuse::socket frontend(queue, zuse::socket_type::router);
	zuse::socket backend(queue, zuse::socket_type::router);
	
	frontend.bind("tcp://*.5555");
	backend.bind("tcp://*.5556");
	
	zuse::state polling("polling");
	
	zuse::message worker_ready("worker is ready", {{"id", zuse::message::any}, {"envelope", ""}, {"ready", "READY"}});
	zuse::message worker_reply("reply received from worker", {{"id", zuse::message::any}, {"envelope", ""}, {"msg", zuse::message::any_frames}});
	zuse::message client_request("request received from client", zuse::message::any_frames);
	
	queue<string> worker_queue;
	
	polling.on_message(worker_ready, [](zuse::context& c) {
		cout << "spqueue: worker ready." << endl;
		worker_queue.push(c.frames().front());
	});
	
	polling.on_message(worker_reply, [](zuse::context& c) {
		cout << "spqueue: received reply from worker." << endl;
		worker_queue.push(c.frames().front());
		cout << "spqueue: forwarding reply to client..." << endl;
		frontend.send(c.frames().begin() + 2, c.frames().end());
	});
	
	polling.on_message(client_request, [&](zuse::context& c) {
		cout << "spqueue: received request from client." << endl;
		auto worker_id = worker_queue.front();
		worker_queue.pop();
		cout << "spqueue: forwarding request to worker..." << endl;
		backend.stream() << worker_id << zuse::endf << zuse::endf << zuse::to_stream(c.frames()) << zuse::endm;
	});
	
	while (true) {
		if (worker_queue.size())
			zuse::poll(queue, {backend, frontend});
		else
			zuse::poll(queue, backend);
	}
	
	return 0;
}









