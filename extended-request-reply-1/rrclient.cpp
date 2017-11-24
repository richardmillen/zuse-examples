//////////////////
// rrclient.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>

using namespace std;

const char* server_addr = "tcp://localhost:5559";

int main(int argc, char* argv[]) {
	zuse::context requester;
	
	cout << "client: connecting to server '" << server_addr << "'..." << endl;
	zuse::socket socket(requester, zuse::socket_type::req);
	socket.connect(server_addr);
	
	zuse::state sending("sending");
	zuse::state recving("receiving");
	zuse::state finished("finished");
	
	zuse::message request("client request", "hello");
	zuse::message reply("server response", "world");
	
	const auto total = 100;
	int count_down = total;

	sending.on_message(request, [&](zuse::context& c) {
		socket.send(c.frames());
	}).next_state(recving);

	recving.on_enter([&](zuse::context& c) {
		requester.execute(socket.recv());
	});

	recving.on_message(reply, [&](zuse::context& c) {
		--count_down;
		cout << "client: received reply #" << (total - count_down) << ", '" << c.frame() << "'." << endl;
	}).next_state({finished, sending});

	finished.add_condition([&](zuse::state& s) {
		return count_down > 0;
	});

	requester.start(sending);
	requester.stop(finished);

	while (requester.is_running()) {
		requester.execute("hello");
	}

	return 0;
}






