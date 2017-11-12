//////////////////
// asyncsrv.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <random>

using namespace std;

#define MATCH_ID		"[0-9A-F]{4}-[0-9A-F]{4}"

int main(int argc, char* argv[]) {
	thread c1(client_node);
	thread c2(client_node);
	thread c3(client_node);
	thread svr(server_node);
	
	c1.join();
	c2.join();
	c3.join();
	svr.join();
	
	cout << "finished!" << endl;
	getchar();
	
	return 0;
}

void client_node() {
	auto id(random_id());
	
	zuse::context_t client;
	
	zuse::state_t ready("sending / receiving");
	
	zuse::message_t request("request to server", R"(request \d+)");
	zuse::message_t response("message from server");
	
	ready.on_message(request, [&](zuse::context_t& c) {
		socket.send(c.frames());
	});
	ready.on_message(response, [&](zuse::context_t& c) {
		cout << "client [" << id << "]: " << c.frame() << endl;
	});
	
	zuse::socket_t socket(client, zuse::socket_type::dealer);
	socket.setsockopt(zuse::opt_type::identity, id);
	socket.connect("tcp://localhost:5570");
	
	client.start(ready);
	
	auto request_nbr = 0;
	while (true) {
		// poll socket 100 times, with timeout of 10 milliseconds:
		client.poll(socket, 100, 10ms);
		
		// send message to fsm as a stream:
		client.stream() << "request #" << ++request_nbr << zuse::endm;
	}
}

void server_node() {
	zuse::context_t server;
	
	zuse::socket_t frontend(server, zuse::socket_type::router);
	zuse::socket_t backend(server, zuse::socket_type::dealer);
	
	frontend.bind("tcp://*:5570", zuse::bind_type::router);
	backend.bind("inproc://backend", zuse::bind_type::dealer);
	
	vector<thread> workers;
	for (auto i = 0; i < 5; ++i)
		workers.push_back(thread(server_worker, &context, i));
	
	server.proxy(frontend, backend);
	
	for (auto& w : workers)
		w.join();
}

// server worker
// 
// note the context_t(context_t&) constructor shown below. the server worker in the zguide 
// example shares the server context as it's thread safe.
// right now it doesn't look like there should be a one-to-one mapping from a zuse to zeromq
// context because each thread is likely to be its own state machine. yet to determine how
// common a multi-threaded state machine actually is. in a nutshell: not sure yet whether 
// zuse's context needs to be thread safe. this piece of code was added as a kind of sentinel;
// at the very least a zuse context will probably need access to the zeromq context held by
// another zuse context.
// 
// note that this isn't exactly a prime candidate for a state machine as it's really only in 
// one state; having a receiving and sending state is a stretch, but this is a (exploratory) 
// example so...
// 
// the context_t::defer() method executes a lambda function after on_message and any state
// transitions have occurred. otherwise calling execute from within on_message would cause
// the on_message associated with the input to be immediately invoked.
void server_worker(zuse::context_t* server, int num) {
	random_device rd;
	mt19937 eng(rd());
	uniform_int_distribution<> rep_distr(1, 5);
	uniform_int_distribution<> sleep_distr(1, 1000);
	
	zuse::context_t worker(*server);
	
	zuse::socket_t socket(worker, zuse::socket_type::dealer);
	socket->connect("inproc://backend");
	
	state_t recving("receiving");
	state_t sending("sending");
	
	zuse::message_t request("request message", {{"id", MATCH_ID}, {"msg", zuse::message_t::any}});
	zuse::message_t reply("loop and send replies", {{"echo-count", R"(\d+)"}, {"id", MATCH_ID}, {"msg", zuse::message_t::any}});		
	
	recving.on_message(request, [&](zuse::event::context_t& w) {
		cout << "server [" << num << "]: echoing (" << w.frame(0) << "," << w.frame(1)  << ")..." << endl;
		
		w.defer([&]() {	w.execute({ rep_distr(env), w.frame(0), w.frame(1) }); });
	}).next_state(sending);
	
	sending.on_message(reply, [&](zuse::event::context_t& w) {
		auto echo_count = stoi(w.frame(0));
		for (auto i = 0; i < echo_count; ++i) {
			this_thread::sleep_for(sleep_distr(eng));
			socket.send(w.frames().begin() + 1, w.frames().end());
		}
	}).next_state(recving);
	
	worker.start(recving);
	worker.execute(socket.recv());
}

string random_id() {
	random_device rd;
	mt19937 eng(rd());
	uniform_int_distribution<> distr(0x00000, 0x10000);
	
	char identity[10] = {};
	sprintf(identity, "%04X-%04X", distr(eng), distr(eng));
	return string(identity);
}







