//////////////////
// hwserver.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>
#include <thread>

using namespace std;

const char* bind_addr = "tcp://*:5555";

int main(int argc, char* argv[]) {
	zuse::context context;
	
	zuse::state sending("sending");
	zuse::state recving("receiving");
	
	zuse::message hello("hello message", "hello");
	zuse::message world("world message", "world");
	
	recving.on_enter([](zuse::context& c) {
		cout << "server: waiting..." << endl;
	});
	
	recving.on_recv(hello, [](zuse::context& c) {
		cout << "server: received '" << c.frame() << "'." << endl;
		
		this_thread::sleep_for(1s);
		c.next_send("world");
	}).next_state(sending);
	
	sending.on_send(world, [](zuse::context& c) {
		cout << "server: sending '" << c.frame() << "'..." << endl;
	}).next_state(recving);
	
	context.bind(bind_addr, zuse::bind_type::rep);
	context.start(recving);
	
	return 0;
}




