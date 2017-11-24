//////////////////
// rrbroker.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>

using namespace std;

const char* frontend_addr = "tcp://*:5559";
const char* backend_addr = "tcp://*:5560";

int main(int argc, char* argv[]) {
	zuse::context broker;
	
	cout << "broker: listening for requests at '" << frontend_addr << "'..." << endl;
	zuse::socket frontend(broker, zuse::socket_type::router);
	frontend.bind(frontend_addr);
	
	cout << "broker: workers allowed to connect to '" << backend_addr << "'..." << endl;
	zuse::socket backend(broker, zuse::socket_type::dealer);
	backend.bind(backend_addr);
	
	zuse::state serving("serving clients and workers");
	
	zuse::message message("any message");

	serving.on_message(message, backend, [](zuse::context& c) {
		frontend.send(c.frames());
	});

	serving.on_message(message, frontend, [](zuse::context& c) {
		backend.send(c.frames());
	});

	broker.start(serving);

	while (broker.is_running())
		zuse::poll(broker, {backend, frontend});

	cout << "broker: shutting down..." << endl;

	return 0;
}





