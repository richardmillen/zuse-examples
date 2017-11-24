//////////////////
// wuserver.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>
#include <random>
using namespace std;

const char* bind_addr = "tcp://*:5556";

int main(int argc, char* argv[]) {
	setfill('0');

	random_device rd;
	mt19937 eng(rd());
	uniform_int_distribution<> code_distr(1, 999);
	uniform_int_distribution<> temp_distr(-5, 90);
	uniform_int_distribution<> hum_distr(0, 100);

	zuse::context publisher;

	zuse::socket socket(publisher, zuse::socket_type::pub);
	socket.bind(bind_addr);

	zuse::state publishing("publishing weather updates");

	zuse::message update("weather update", R"(^\d{3} -?(?!\d{3})\d+ (?!\d{4})\d+)");

	publishing.on_enter([](zuse::context& c) {
		cout << "server: sending data to subscribers..." << endl;
	});

	publishing.on_message(update, [](zuse::context& c) {
		socket.send(c.frames());
	});

	publisher.start(publishing);

	while (publisher.is_running()) {
		zuse::stream(publisher) 
			<< setw(3) << code_distr(eng) 
			<< " "
			<< temp_distr(eng)
			<< " "
			<< hum_distr(eng)
			<< zuse::endm;
	}

	return 0;
}





