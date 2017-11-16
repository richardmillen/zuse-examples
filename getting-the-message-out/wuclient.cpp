////////////////// 
// wuclient.cpp //
////////////////// 

#include "zuse.hpp"

#include <iostream>
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
	
	zuse::message_t update("update", R"()");







	return 0;
}














