//////////////////
// asyncsrv.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>
#include <thread>
#include <mutex>
#include <random>

using namespace std;

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
	
	recving.on_enter([&](zuse::context_t& c) {
		recv_count = 0;
	});
	recving.on_recv(inbound, [&](zuse::context_t& c) {
		// TODO: print received message
		++recv_count;
	}).next_state(sending);
	recving.on_exit([](zuse::context_t& c) {
		c.next_stream() << "request " << ++req_count;
	});
	
	sending.add_condition([&](zuse::state_t& s) {
		return recv_count >= 100;
	});
	
	sending.on_send(request, [](zuse::context_t& c) {
		cout << "client: sending '" << c.frame() << "'..." << endl;
	}).next_state(recving);
	
	context.connect("tcp://localohst:5570", zuse::connect_type::dealer, random_id());
	
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

void server_work(zuse::context_t* context, int num) {
	auto id(to_string(num));
	
	random_device rd;
	mt19937 eng(rd());
	uniform_int_distribution<> rep_distr(1, 5);
	uniform_int_distribution<> sleep_distr(1, 1000);
	
	state_t recving("receiving");
	state_t sending("sending");
	
	auto worker = context->connect("inproc://backend", zuse::connect_type::dealer);
	
	zuse::message_t message("request/reply message", {zuse::message_t::any, zuse::message_t::any});
	
	// TODO: define this is a type that indicates that it's private i.e. won't be sent over the socket
	zuse::message_t loop("loop and send replies", {R"(\d+)", zuse::message_t::any, zuse::message_t::any});		
	
	recving.on_recv(worker, message, [](zuse::context_t& c) {
		cout << "server [" << id << "]: echoing (" << c.frame(0) << "," << c.frame(1)  << ")..." << endl;
		c.next_send(rep_distr(env));
		c.next_send(c.frames());
	}).next_state(sending);
	
	sending.on_send(loop, [&](zuse::context_t& c) {
		auto echo_count = stoi(c.private_frame());
		for (auto i = 0; i < echo_count; ++i) {
			this_thread::sleep_for(sleep_distr(eng));
			c.send(c.frames());
		}
	});
	
	sending.on_send(worker, message, [&](zuse::context_t& c) {
	});
}
















