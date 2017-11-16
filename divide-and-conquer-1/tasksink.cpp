//////////////////
// tasksink.cpp //
//////////////////

#include "zuse.hpp"

#include <iostream>
#include <thread>
#include <chrono>
using namespace std;

const char* recv_addr = "tcp://*:5558";
const char* ctrl_addr = "tcp://*:5559";

int main(int argc, char* argv[]) {
	zuse::context_t sink;
	
	cout << "sink: binding to receiver address '" << recv_addr << "'..." << endl;
	zuse::socket_t receiver(sink, zuse::socket_type::pull);
	receiver.bind(recv_addr);
	
	cout << "sink: binding to controller address '" << ctrl_addr << "'..." << endl;
	zuse::socket_t controller(sink, zuse::socket_type::pub);
	controller.bind(ctrl_addr);
	
	zuse::state_t waiting("waiting for go ahead");
	zuse::state_t receiving("receiving results");
	zuse::state_t finished("finished receiving");
	
	zuse::message_t go("go ahead from ventilator", "");
	zuse::message_t result("result", R"(\d+)");
	zuse::message_t kill("kill workers", "KILL");
	
	auto task_num = 0u;
	auto total_ms = 0u;
	
	waiting.on_enter([&](zuse::context_t& c) {
		cout << "sink: waiting for the go ahead from the ventilator..." << endl;
		sink.execute(receiver.recv());
	});
	waiting.on_message(go, [](zuse::context_t& c) {
		cout << "sink: ready to receive results." << endl;
	}).next_state(receiving);
	
	receiving.on_message(result, [](zuse::context_t& c) {
		++task_num;
		if (task_num / 10 == 0)
			cout << ":" << flush;
		else
			cout << "." << flush;
	}).next_state(finished);
	
	finished.add_condition([&](zuse::context_t& c) {
		return task_num == 100;
	});
	finished.on_message(kill, [](zuse::context_t& c) {
		cout << "sink: sending 'KILL' message to workers..." << endl;
		controller.send(c.frames());
	});
	
	sink.start(waiting);
	
	auto start = chrono::system_clock::now();
	
	while (!sink.is_current(finished)) {
		sink.execute(receiver.recv());
	}
	
	auto end = chrono::system_clock::now();
	
	chrono::duration<double> elapsed_secs = end - start;
	time_t end_time = chrono::system_clock::to_time_t(end);
	
	cout << endl << "sink: finished at " << ctime(&end_time) <<
		". elapsed time: " << elapsed_secs.count() << "s" << endl;
	
	sink.execute("KILL");
	
	return 0;
}









