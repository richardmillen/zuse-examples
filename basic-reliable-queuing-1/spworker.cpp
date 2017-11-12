//////////////////
// spworker.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>
#include <string>
#include <random>
#include <thread>

using namespace std;

#ifdef WIN32
#include <process.h>
#define getpid()		::_getpid()
#else
#define getpid()		::getpid()
#endif

int main(int argc, char* argv[]) {
	zuse::context_t worker;
	
	zuse::socket_t socket(worker, zuse::socket_type::req);
	auto id = set_id(socket);
	socket.connect("tcp://localhost:5556");
	
	worker.on_shutdown([]() {
		cout << "spworker [" << id << "]: shutting down..." << endl;
	});
	
	zuse::state_t not_ready("worker is starting");
	zuse::state_t ready("worker is ready");
	zuse::state_t working("worker is working");
	zuse::state_t replying("worker is sending results of job");
	zuse::state_t crash("worker has crashed");
	zuse::state_t overload("cpu overload");
	
	random_device rd;
	mt19937 eng(rd());
	uniform_int_distribution<> distr(0, 5);
	auto job_count = 0;
	
	crash.add_condition([&](zuse::state_t& s) {
		return job_count > 10 && distr(eng) == 0;
	});
	
	overload.add_condition([&](zuse::state_t& s) {
		return job_count > 5 && distr(eng) == 0;
	});
	
	zuse::message_t ready("worker is ready", "READY");
	// TODO: properly define accepted job / result messages
	zuse::message_t job("job request");
	zuse::message_t result("job result");
	
	not_ready.on_message(ready, [&](zuse::context_t& w) {
		cout << "spworker [" << id << "]: sending 'ready' message to broker..." << endl;
		socket.send(w.frames());
	}).next_state(ready);
	
	ready.on_enter([&](zuse::context_t& w) {
		w.execute(socket.recv());
	});
	
	ready.on_message(job, [&](zuse::context_t& w) {
		++job_count;
		w.defer([&]() {	w.execute(w.frames()); });
	}).next_state({crash, overload, working});
	
	crash.on_message(job, [&](zuse::context_t& w) {
		cout << "spworker [" << id << "]: simulating crash (by not returning to 'ready' state)..." << endl;
	});
	overload.on_message(job, [&](zuse::context_t& w) {
		cout << "spworker [" << id << "]: simulating cpu overload (before transition to 'working' state)..." << endl;
	}).next_state(working);
	
	working.on_message(job, [&](zuse::context_t& w) {
		cout << "spworker [" << id << "]: doing work..." << endl;
		this_thread::sleep_for(1s);
		w.defer([&]() { worker.execute(w.frames()) });
	}).next_state(replying);
	
	replying.on_message(result, [&](zuse::context_t& w) {
		cout << "spworker [" << id << "]: normal reply - '" << frames.back() << "'." << endl;
		socket.send(w.frames());
	}).next_state(ready);
	
	worker.start(not_ready);
	
	worker.execute("READY");
	
	return 0;
}

string set_id(zuse::socket_t& socket) {
	string id("spworker-" + to_string(getpid()));
	socket.setsockopt(ZMQ_IDENTITY, id);
	return id;
}










