//////////////////
// hwserver.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>

using namespace std;

const char* bind_addr = "tcp://*:5555";

int main(int argc, char* argv[]) {
	zuse::context context;
	zuse::socket socket(context, zuse::socket_type::rep);
	
	zuse::state ready("receiving");
	
	zuse::message request("client request", "hello");
	
	ready.on_message(request, [&](zuse::context& c) {
		socket.send("world");
	});
	
	socket.bind(bind_addr);
	context.start(ready);
	
	while (true)
		context.execute(socket.recv());
}









