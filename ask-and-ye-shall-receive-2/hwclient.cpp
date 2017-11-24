//////////////////
// hwclient.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>

using namespace std;

const char* server_addr = "tcp://localhost:5555";

int main(int argc, char* argv[]) {
	zuse::context context;
	
	zuse::state sending("sending");
	zuse::state recving("receiving");
	
	zuse::message hello("hello message", "hello");
	zuse::message world("world message", "world");
	
	sending.on_send(hello, [](zuse::context& c) {
		cout << "client: sending '" << c.frame() << "'..." << endl;
	}).next_state(recving);
		
	recving.on_recv(world, [](zuse::context& c) {
		cout << "client: received '" << c.frame() << "'." << endl;
	}).next_state(sending);
	
	context.connect(server_addr, zuse::connect_type::req);
	context.start(sending);
	
	for (auto request_nbr = 0; request_nbr < 10; ++request_nbr) {
		context.send("hello");
	}
	
	return 0;
}






