
#include <iostream>
#include <exception>
#include <stdexcept>
#include <deque>

#include "config.hpp"
#include "util/util.hpp"
#include "communication/worker.hpp"
#include "worker_task.hpp"

MPI_Info info = MPI_INFO_NULL;

auto sleep_duration_between_polls = std::chrono::milliseconds(50);


int main(int argc, char **argv)
{
	try {
		#if DEBUG_WORKER
			std::cout << "worker started with arguments";
			for (int i = 0; i < argc; i++) { std::cout << " '" << argv[i] << "'"; }
			std::cout << std::endl;
		#endif

		if (argc < 2)
			throw std::runtime_error("name for problem instance required!");
		
		const char *server_name = argv[1];
		
		// MPI_Init(&argc, &argv);
		int provided;
		MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided); // MPI_THREAD_FUNNELED
		if (provided < MPI_THREAD_MULTIPLE) {
			throw std::runtime_error("MPI does not provide needed threading level!");
		}

		{
			int is_initialized;
			MPI_Initialized(&is_initialized);
			assert(0 != is_initialized);
		}

		// unsigned int num_workers = std::max((int)std::thread::hardware_concurrency() - 1, 1);
		unsigned int num_workers = NUM_PROBSAT_THREADS_PER_WORKER;

		int length = 0;
		char processor_name[MPI_MAX_PROCESSOR_NAME];
		MPI_Get_processor_name(processor_name, &length);
		
		#if DEBUG_WORKER
			std::cout << msg_header << "running on " << processor_name << " and will spawn " << num_workers << " threads" << std::endl;
		#endif

		MPI_Comm_get_parent(&manager_cl);
		
		if (MPI_COMM_NULL == manager_cl)
			throw std::runtime_error("worker needs to be spawned by a manager process");

		MPI_Info_create(&info);

		#if DEBUG_WORKER
			std::cout << msg_header << "connecting to " << server_name << std::endl;
		#endif
		
		char port_name[MPI_MAX_PORT_NAME];
		assert(MPI_SUCCESS == MPI_Lookup_name(server_name, info, port_name));

		#if DEBUG_COMMUNICATION
			std::cout << msg_header << "connecting to port name " << port_name << std::endl;
		#endif

		assert(MPI_SUCCESS == MPI_Comm_connect(port_name, info, 0, MPI_COMM_SELF, &server_cl));

		recieve_server_cmd(S2W::ACTIVATE_WORKER);

		#if DEBUG_WORKER
			std::cout << msg_header << "activated, starting " << num_workers << " threads!" << std::endl;
		#endif

		// using namespace std::chrono_literals;
		// std::this_thread::sleep_for(500ms);

		std::vector<std::thread> workers;
		for (uint i = 0; i < num_workers; i++) {
			workers.push_back(std::thread(worker_func));
		}

		instances_to_get = num_workers;
		auto instances_buffered = instances_to_get - num_workers;

		while (running) {
			std::shared_ptr<task> t;
			bool task_to_return = false;
			{ // tasks_done_mutex.lock();
				std::lock_guard<std::mutex> lock(tasks_done_mutex);

				task_to_return = !tasks_done.empty();
				if (task_to_return) {
					t = tasks_done.front();
					tasks_done.pop_front();
				}
			} // tasks_done_mutex.unlock();

			if (task_to_return) {
				return_task(t);
			} else {
				bool nothing_done = true;

				// nonblocking probe of available manager commands
				command_buffer.clear();

				MPI_Status status;
				int flag = 0;
				MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, server_cl, &flag, &status);
				if (0 != flag) {
					process_server_cmd(status);
					// recieve_server_cmd();
					nothing_done = false;
				}

				if ((running) && (std::min(instances_buffered, num_workers) < instances_to_get)) {
					#if DEBUG_WORKER
						std::cout << msg_header << "requesting " << instances_to_get << " new instances" << std::endl;
					#endif
					W2S::get_instances gi_cmd({instances_to_get});
					gi_cmd.send();

					auto old_instances_to_get = instances_to_get;
					recieve_server_cmd(S2W::SEND_INSTANCES);

					if (instances_to_get != old_instances_to_get) {
						nothing_done = false;
					}
				}
				
				if (nothing_done) {
					if (0 < sleep_duration_between_polls.count()) {
						// #if DEBUG_WORKER
							// std::cout << msg_header << " is going to sleep" << std::endl;
						// #endif

						{ // tasks_done_mutex.lock();
							std::unique_lock<std::mutex> lock(tasks_done_mutex);
							if (0 >= instances_to_get) {
								cv_main_mpi_thread.wait_for(lock, sleep_duration_between_polls);
							}
						} // tasks_done_mutex.lock();
					}
				}
			}

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(50ms);
		}
		
		for (uint i = 0; i < workers.size(); i++) {
			workers[i].join();
		}

		while (!tasks_done.empty()) {
			auto t = tasks_done.front();
			tasks_done.pop_front();
			return_task(t);
		}
	} catch (const std::exception& ex) {
		print_exception("worker", ex);
	}

	try {
		W2S::disconnect::send();

		// using namespace std::chrono_literals;
		// std::this_thread::sleep_for(5s);
		MPI_Barrier(server_cl);
		MPI_Comm_disconnect(&server_cl);
		// MPI_Comm_disconnect(&manager_cl);


		MPI_Barrier(manager_cl);
		MPI_Comm_disconnect(&manager_cl);

		// using namespace std::chrono_literals;
		// std::this_thread::sleep_for(50ms);
		
		int is_initialized;
		MPI_Initialized(&is_initialized);
		// if (is_initialized) {
		assert(is_initialized);
		// MPI_Finalize();
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_T_finalize();
		
		// std::this_thread::sleep_for(50ms);
	} catch (const std::exception& ex) {
		print_exception("worker", ex);
	}

	#if DEBUG_WORKER
		std::cout << "worker finished" << std::endl;
	#endif

	return EXIT_SUCCESS;
}
