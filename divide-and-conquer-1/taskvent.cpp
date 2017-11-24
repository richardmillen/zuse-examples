//////////////////
// taskvent.cpp //
//////////////////

#include "zuse.hpp"
#include "zuse/stream.hpp"

#include <iostream>
#include <random>
#include <algorithm>
#include <thread>
using namespace std;

const char* bind_addr = "tcp://*:5557";
const char* sink_addr = "tcp://localhost:5558";

int main(int argc, char* argv[]) {
	random_device rd;
	mt19937 eng(rd());
	uniform_int_distribution<> distr(1, 100);
	
	zuse::context ventilator;
	
	zuse::socket sender(ventilator, zuse::socket_type::push);
	sender.bind(bind_addr);
	
	zuse::socket sink(context, zuse::socket_type::push);
	sink.connect(sink_addr);
	
	zuse::state waiting("waiting for workers");
	zuse::state sending("sending tasks");
	zuse::state finished("finished sending tasks");
	
	zuse::message wait("wait", "wait");
	zuse::message wakeup("wake sink", "0");
	zuse::message task("task", R"(\d+)");
	
	waiting.on_message(wait, [](zuse::context& c) {
		cout << "vent: press enter when workers are ready: " << endl;
		getchar();
	}).next_state(sending);
	
	sending.on_message(wakeup, [](zuse::context& c) {
		cout << "vent: sending task to worker..." << endl << endl;
		sink.send(c.frame());
	});
	
	auto task_num = 0u;
	auto total_ms = 0u;
	
	sending.on_message(task, [&](zuse::context& c) {
		++task_num;
		
		auto workload = stoi(c.frame());
		total_ms += workload;
		
		zuse::stream ms(sender)
		ms << workload << zuse::endm;
	}).next_state(finished);
	
	finished.add_condition([&](zuse::state& s) {
		return task_num == 100;
	});
	finished.on_enter([&](zuse::context& c) {
		cout << "vent: total expected cost: " << total_ms << "ms" << endl;
		this_thread::sleep_for(1s);
	});
	
	ventilator.start(waiting);
	ventilator.stop(finished);
	
	ventilator.execute("wait");
	ventilator.execute("0");
	
	while (ventilator.is_running()) {
		ventilator.execute(distr(eng));
	}
	
	return 0;
}






