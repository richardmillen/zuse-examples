////////////////// 
// wuclient.cpp //
////////////////// 

#include "zuse.hpp"

#include <iostream>
#include <sstream>
using namespace std;

const char* filter = "001";
const char* server_addr = "tcp://localhost:5556";

int main(int argc, char* argv[]) {
	zuse::context_t client;

	zuse::socket_t socket(client, zuse::socket_type::sub);
	socket.connect(server_addr);
	socket.setsocketopt(ZMQ_SUBSCRIBE, filter, strlen(filter));

	zuse::state_t recving("receiving");
	zuse::state_t finished("finished");
	
	zuse::message_t update("update", R"(^\d{3} -?(?!\d{3})\d+ (?!\d{4})\d+)");

	auto update_num = 0;
	auto total_temp = 0L;

	recving.on_message(update, [](zuse::context_t& c) {
		cout << "client: received update." << endl;

		istringstream iss(c.frame());

		int code, temperature, relhumid;
		iss >> code >> temperature >> relhumid;

		total_temp += temperature;
		++update_num;
	}).next_state(finished);

	finished.add_condition([&](zuse::state_t& s) {
		return update_num == 100;
	});

	client.start(recving);
	client.stop(finished);

	while (client.is_running())
		client.execute(socket.recv());

	return 0;
}














