//////////////////
// rrworker.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>
#include <thread>

using namespace std;

const char* broker_addr = "tcp://localhost:5560";

int main(int argc, char* argv[]) {
	zuse::context_t worker;
	
	cout << "worker: connecting to broker '" << broker_addr << "'..." << endl;
	zuse::socket_t socket(worker, zuse::socket_type::rep);
	socket.connect(broker_addr);

	zuse::state_t recving("receiving");
	zuse::state_t working("working");
	zuse::state_t sending("replying");

	zuse::message_t request("request", "hello");
	zuse::message_t reply("reply", "world");
	
	recving.on_enter([](zuse::context_t& c) {
		c.execute(socket.recv());
	});
	recving.on_message(request, [](zuse::context_t& c) {
		cout << "worker: received request '" << c.frame() << "'." << endl;
		c.defer([&]() { c.execute(c.frames()); });
	}).next_state(working);

	working.on_message(request, [](zuse::context_t& c) {
		this_thread::sleep_for(1s);
	}).next_state(sending);

	sending.on_enter([](zuse::context_t& c) {
		c.execute("world");
	});
	sending.on_message([](zuse::context_t& c) {
		socket.send(c.frames());
	}).next_state(recving);

	worker.start(recving);
}






