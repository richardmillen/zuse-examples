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
	zuse::context sink;
	
	cout << "sink: binding to receiver address '" << recv_addr << "'..." << endl;
	zuse::socket receiver(sink, zuse::socket_type::pull);
	receiver.bind(recv_addr);
	
	cout << "sink: binding to controller address '" << ctrl_addr << "'..." << endl;
	zuse::socket controller(sink, zuse::socket_type::pub);
	controller.bind(ctrl_addr);
	
	zuse::state waiting("waiting for go ahead");
	zuse::state receiving("receiving results");
	zuse::state finished("finished receiving");
	
	zuse::message go("go ahead from ventilator", "");
	zuse::message result("result", R"(\d+)");
	zuse::message kill("kill workers", "KILL");
	
	auto task_num = 0u;
	auto total_ms = 0u;
	
	waiting.on_enter([&](zuse::context& c) {
		cout << "sink: waiting for the go ahead from the ventilator..." << endl;
		sink.execute(receiver.recv());
	});
	waiting.on_message(go, [](zuse::context& c) {
		cout << "sink: ready to receive results." << endl;
	}).next_state(receiving);
	
	receiving.on_message(result, [](zuse::context& c) {
		++task_num;
		if (task_num / 10 == 0)
			cout << ":" << flush;
		else
			cout << "." << flush;
	}).next_state(finished);
	
	finished.add_condition([&](zuse::context& c) {
		return task_num == 100;
	});
	finished.on_message(kill, [](zuse::context& c) {
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
	time end_time = chrono::system_clock::to_time(end);
	
	cout << endl << "sink: finished at " << ctime(&end_time) <<
		". elapsed time: " << elapsed_secs.count() << "s" << endl;
	
	sink.execute("KILL");
	
	return 0;
}









