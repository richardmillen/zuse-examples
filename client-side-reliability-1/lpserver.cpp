//////////////////
// lpserver.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>
#include <random>
#include <chrono>
#include <thread>

using namespace std;

const char* bind_addr = "tcp://*:5555";

int main(int argc, char* argv[]) {
	auto cycles = 0;
	
	random_device rd;
	mt19937 eng(rd());
	uniform_int_distribution<> crash_distr(0, 29);
	uniform_int_distribution<> slow_distr(0, 9);
	
	zuse::context server;
	
	zuse::socket socket(server, zuse::socket_type::rep);
	socket.bind(bind_addr);
	
	zuse::state serving("serving");
	
	zuse::message request("outbound", R"(\d+)");
	zuse::message reply("server response", request);
	
	serving.on_message(request, [&](zuse::context& c) {
		++cycles;
		
		if (cycles > 3 && crash_distr(eng) == 0) {
			cout << "lpserver: simulating crash..." << endl;
			c.abort();
			return;
		} else if (cycles > 3 && slow_distr(eng) == 0) {
			cout << "lpserver: simulating cpu overload..." << endl;
			this_thread::sleep_for(2s);
		}
		
		cout << "lpserver: handling request (" << c.frame() << ")..." << endl;
		this_thread::sleep_for(1s);
		
		c.execute(c.frame());
	});
	
	serving.on_message(reply, [&](zuse::context& c) {
		cout << "lpserver: sending reply..." << endl;
		socket.send(c.frames());
	}
	
	server.start(serving);
	
	while (server.is_running()) {
		server.execute(socket.recv());
	}
	
	return 0;
}




















