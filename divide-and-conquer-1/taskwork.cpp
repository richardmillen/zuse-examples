//////////////////
// taskwork.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>
#include <thread>
using namespace std;

const char* vent_addr = "tcp://localhost:5557"
const char* sink_addr = "tcp://localhost:5558"
const char* ctrl_addr = "tcp://localhost:5559"

int main(int argc, char* argv[]) {
	zuse::context worker;
	
	cout << "work: connecting to ventilator '" << vent_addr << "'..." << endl;
	zuse::socket receiver(worker, zuse::socket_type::pull);
	receiver.connect(vent_addr);
	
	cout << "work: connecting to sink '" << sink_addr << "'..." << endl;
	zuse::socket sender(worker, zuse::socket_type::push);
	sender.connect(sink_addr);
	
	cout << "work: connecting to sink control port '" << ctrl_addr << "'..." << endl;
	zuse::socket controller(worker, zuse::socket_type::sub);
	controller.connect(ctrl_addr);
	controller.setsockopt(ZMQ_SUBSCRIBE, "", 0);
	
	zuse::state polling("polling for tasks");
	zuse::state working("performing task");
	zuse::state finished("finished all tasks");
	
	zuse::message task("task", R"(\d+)");
	zuse::message no_more("no more tasks");
	
	polling.on_message(task, receiver, [](zuse::context& c) {
		c.defer([&]() { c.execute(c.frames()); });
	}).next_state(working);
	
	working.on_message(task, [&](zuse::context& c) {
		auto workload = stoi(c.frame());
		this_thread::sleep_for(workload, chrono::milliseconds);
		sender.send(c.frames());
		
		cout << "." << flush;
	}).next_state(polling);
	
	polling.on_message(no_more, controller, [](zuse::context& c) {
		cout << endl << "work: finished!" << endl;
	}).next_state(finished);
	
	worker.start(waiting);
	worker.stop(finished);
	
	while (worker.is_running()) {
		zuse::poll({receiver, controller});
	}
	
	return 0;
}














