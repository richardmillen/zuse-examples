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

#define MATCH_ID	"[0-9A-F]{4}-[0-9A-F]{4}"

mutex mut;

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
	zuse::context_t context;
	
	zuse::state_t recving("receiving");
	zuse::state_t sending("sending");
	
	zuse::message_t inbound("any message from server");
	zuse::message_t request("request to server", R"(request \d+)");
	
	unsigned req_count;
	auto recv_count = 0;
	auto id = random_id();
	
	recving.on_enter([&](zuse::event::context_t& c) {
		recv_count = 0;
	});
	recving.on_recv(inbound, [&](zuse::event::context_t& c) {
		cout "client [" << id << "]: " << c.frame() << endl;
		++recv_count;
	}).next_state(sending);
	recving.on_exit([&](zuse::event::context_t& c) {
		c.defer([&]() { c.send() << "request " << ++req_count; });
	});
	
	sending.add_condition([&](zuse::state_t& s) {
		return recv_count >= 100;
	});
	
	sending.on_send(request, [](zuse::event::context_t& c) {
		cout << "client [" << id << "]: sending '" << c.frame() << "'..." << endl;
	}).next_state(recving);
	
	context.connect("tcp://localohst:5570", zuse::connect_type::dealer, id);
	
	context.poll(recving, 10ms);
}

void server_node() {
	zuse::context_t context;
	
	context.bind("tcp://*:5570", zuse::bind_type::router);
	context.bind("inproc://backend", zuse::bind_type::dealer);
	
	vector<thread> workers;
	for (auto i = 0; i < 5; ++i)
		workers.push_back(thread(server_work, &context, i));
	
	context.proxy();
	
	for (auto& w : workers)
		w.join();
}

// server worker thread.
//
// a zmq socket is paired with the thread in which it's created (context->connect). 
// this means that unless you call connect more than once in a thread you don't need
// to work with the socket as zuse will use handle that for you.
//
// the worker can be in two states; receiving or sending. this was really done for 
// demonstration purposes as the code is so simple it only needs a single state to
// receive requests and send replies.
void server_work(zuse::context_t* context, int num) {
	random_device rd;
	mt19937 eng(rd());
	uniform_int_distribution<> rep_distr(1, 5);
	uniform_int_distribution<> sleep_distr(1, 1000);
	
	state_t recving("receiving");
	state_t sending("sending");
	
	context->connect("inproc://backend", zuse::connect_type::dealer);
	
	zuse::message_t request("request message", {{"id", MATCH_ID}, {"msg", zuse::message_t::any}});
	zuse::message_t reply("loop and send replies", {{"echo-count", R"(\d+)"}, {"id", MATCH_ID}, {"msg", zuse::message_t::any}});		
	
	recving.on_recv(request, [](zuse::event::context_t& c) {
		cout << "server [" << num << "]: echoing (" << c.frame(0) << "," << c.frame(1)  << ")..." << endl;
		
		c.defer([&]() {	c.raise_event({ rep_distr(env), c.frame(0), c.frame(1) }); });
	}).next_state(sending);
	
	sending.on_event(reply, [&](zuse::event::context_t& c) {
		auto echo_count = stoi(c.frame(0));
		for (auto i = 0; i < echo_count; ++i) {
			this_thread::sleep_for(sleep_distr(eng));
			c.send(c.frames().begin() + 1, c.frames().end());
		}
	}).next_state(recving);
}

string random_id() {
	random_device rd;
	mt19937 eng(rd());
	uniform_int_distribution<> distr(0x00000, 0x10000);
	
	char identity[10] = {};
	sprintf(identity, "%04X-%04X", distr(eng), distr(eng));
	return string(identity);
}






















