//////////////////
// lpclient.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>

using namespace std;

#define MAX_RETRIES		3
#define TIMEOUT_SECS		3s

int main(int argc, char* argv[]) {
	zuse::context client;
	auto socket = make_socket(client);
	
	auto sequence = 0;
	auto retries_left = MAX_RETRIES;
	
	zuse::state_t sending("sending");
	zuse::state_t recving("receiving");
	
	no_server.add_condition([&](zuse::state_t& s) {
		return --retries_left == 0;
	});
	
	zuse::message_t out("outbound", R"(\d+)");
	zuse::message_t response("server response", out);
	
	sending.on_enter([&](zuse::context_t& c) {
		cout << "lpclient: sending request " << ++sequence << " to server..." << endl;
		client.stream() << sequence << zuse::endm;
	}
	
	sending.on_message(out, [&](zuse::context_t& c) {
		socket.send(c.frames());
	}).next_state(recving);
	
	recving.on_enter([&](zuse::context_t& c) {
		client.poll(socket, MAX_RETRIES, TIMEOUT_SECS);
	});
	
	recving.on_message(response, [&](zuse::context_t& c) {
		auto reply = stoi(c.frame());
		
		if (reply != sequence) {
			cerr << "lpclient: invalid reply from server. expected: " << sequence << ", but was " << reply << "." << endl;
			c.abort();
			return;
		}
		
		cout << "lpclient: server replied - " << reply << "." << endl;
		retries_left = MAX_RETRIES;
	}).next_state(sending);
	
	recving.on_timeout([&](zuse::context_t& c) {
		cerr << "lpclient: no response from server, retrying..." << endl;
		
		socket = make_socket(context);
		socket.stream() << sequence << zuse::endm;
	});
	
	recving.on_failed([&](zuse::context_t& c) {
		cerr << "lpclient: server seems to be offline, abandoning..." << endl;
		c.abort();
	})
	
	client.start(sending);
	
	return 0;
}

zuse::socket_t make_socket(zuse::context_t& context) {
	zuse::socket_t socket(context, zuse::socket_type::req);
	socket.connect("tcp://localhost:5555");
	client.setsockopt(ZMQ_LINGER, false);
	return client;
}








