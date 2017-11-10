//////////////////
// hwclient.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>

using namespace std;

const char* server_addr = "tcp://localhost:5555";

int main(int argc, char* argv[]) {
	zuse::context_t context;
	
	zuse::state_t sending("sending");
	zuse::state_t recving("receiving");
	
	zuse::message_t hello("hello message", "hello");
	zuse::message_t world("world message", "world");
	
	sending.on_send(hello, [](zuse::context_t& c) {
		cout << "client: sending '" << c.frame() << "'..." << endl;
	}).next_state(recving);
		
	recving.on_recv(world, [](zuse::context_t& c) {
		cout << "client: received '" << c.frame() << "'." << endl;
	}).next_state(sending);
	
	context.connect(server_addr, zuse::connect_type::req);
	context.start(sending);
	
	for (auto request_nbr = 0; request_nbr < 10; ++request_nbr) {
		context.send("hello");
	}
	
	return 0;
}






