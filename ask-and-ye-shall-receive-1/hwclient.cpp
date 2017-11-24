//////////////////
// hwclient.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>

using namespace std;

const char* server_addr = "tcp://localhost:5555";

int main(int argc, char* argv[]) {
	zuse::context context;
	zuse::socket socket(context, zuse::socket_type::req);
	
	zuse::state sending("sending");
	zuse::state recving("receiving");
	
	zuse::message request("client request", "hello");
	zuse::message reply("server response", "world");
	
	sending.on_message(request, [&](zuse::context& c) {
		cout << "client: sending '" << c.frame() << "'..." << endl;
		socket.send(c.frames());
	}).next_state(recving);
	
	recving.on_message(reply, [](zuse::context& c) {
		cout << "client: received '" << c.frame() << "'." << endl;
	}).next_state(sending);
	
	socket.connect(server_addr);
	context.start(sending);
	
	for (auto request_nbr = 0; request_nbr < 10; ++request_nbr) {
		context.execute("hello");
		
		context.execute(socket.recv());
	}
	
	return 0;
}






