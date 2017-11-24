//////////////////
// rrworker.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>
#include <thread>

using namespace std;

const char* broker_addr = "tcp://localhost:5560";

int main(int argc, char* argv[]) {
	zuse::context worker;
	
	cout << "worker: connecting to broker '" << broker_addr << "'..." << endl;
	zuse::socket socket(worker, zuse::socket_type::rep);
	socket.connect(broker_addr);

	zuse::state recving("receiving");
	zuse::state working("working");
	zuse::state sending("replying");

	zuse::message request("request", "hello");
	zuse::message reply("reply", "world");
	
	recving.on_enter([](zuse::context& c) {
		c.execute(socket.recv());
	});
	recving.on_message(request, [](zuse::context& c) {
		cout << "worker: received request '" << c.frame() << "'." << endl;
		c.defer([&]() { c.execute(c.frames()); });
	}).next_state(working);

	working.on_message(request, [](zuse::context& c) {
		this_thread::sleep_for(1s);
	}).next_state(sending);

	sending.on_enter([](zuse::context& c) {
		c.execute("world");
	});
	sending.on_message([](zuse::context& c) {
		socket.send(c.frames());
	}).next_state(recving);

	worker.start(recving);
}






