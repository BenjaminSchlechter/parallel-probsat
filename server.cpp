#include <iostream>
#include <exception>
#include <stdexcept>
#include <deque>
#include <thread>
#include <chrono>

#include "config.hpp"
#include "util/util.hpp"
#include "communication/server.hpp"

std::vector<uint8_t> tmp_buffer;


int main(int argc, char **argv)
{
	bool published = false;

	try {
		#if DEBUG_COMMUNICATION
			std::cout << "server started with arguments";
			for (int i = 0; i < argc; i++) { std::cout << " '" << argv[i] << "'"; }
			std::cout << std::endl;
		#endif

		if (argc < 2)
			throw std::runtime_error("name for problem instance required!");
		
		server_name = argv[1];

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

		int length = 0;
		char processor_name[MPI_MAX_PROCESSOR_NAME];
		MPI_Get_processor_name(processor_name, &length);
		#if DEBUG_COMMUNICATION
			std::cout << msg_header << "running on " << processor_name << std::endl;
		#endif

		MPI_Comm_get_parent(&manager_cl);
		
		if (MPI_COMM_NULL == manager_cl)
			throw std::runtime_error("server needs to be spawned by a manager process");

		// int num_parents;
		// MPI_Comm_remote_size(manager_cl, &num_parents);

		// if (1 != num_parents)
			// throw std::runtime_error("server needs exactly one manager as parent");

		MPI_Info_create(&server_info);

		MPI_Open_port(server_info, port_name);
		#if DEBUG_COMMUNICATION
			std::cout << msg_header << " port name is " << port_name << std::endl;
		#endif

		MPI_Publish_name(server_name, server_info, port_name);
		published = true;

		while (running) {
			// nonblocking probe of available worker commands
			if (!work_unit_requests.empty()) {
				MPI_Status status;
				int index = 0;
				int flag = 0;
				assert(MPI_SUCCESS == MPI_Testany(work_unit_requests.size(), work_unit_requests.data(), &index, &flag, &status));

				if (flag) {
					assert(index != MPI_UNDEFINED);
					if ((0 > index) || ((std::size_t) index >= work_unit_requests.size())) {
						std::cout << "valid range: [0, " << (work_unit_requests.size()-1) << "]" << std::endl;
						throw std::runtime_error(std::string("fatal error: MPI_Testany returned invalid index ")
							+ std::to_string(index) + std::string("!"));
					}

					process_worker_cmd(index, status);
				}
			}

			// nonblocking probe of available manager commands
			MPI_Status status;
			int flag = 0;
			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, manager_cl, &flag, &status);
			if (flag) {
				process_manager_cmd(status);
			}
		}
	} catch (const std::exception& ex) {
		print_exception("server", ex);
	}

	try {
		if (published) {
			MPI_Unpublish_name(server_name, MPI_INFO_NULL, port_name);
		}

		S2M::disconnect::send();

		MPI_Barrier(manager_cl);
		MPI_Comm_disconnect(&manager_cl);

		MPI_Barrier(MPI_COMM_WORLD);

		MPI_Barrier(MPI_COMM_WORLD);

		int is_initialized;
		MPI_Initialized(&is_initialized);
		assert(is_initialized);
		// MPI_Finalize();
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_T_finalize();
		
		// using namespace std::chrono_literals;
		// std::this_thread::sleep_for(100ms);
	} catch (const std::exception& ex) {
		print_exception("server", ex);
	}

	#if DEBUG_COMMUNICATION
		std::cout << "server finished" << std::endl;
	#endif

	return EXIT_SUCCESS;
}

