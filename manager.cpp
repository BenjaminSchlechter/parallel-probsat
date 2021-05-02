
/*
 * a command line wrapper to manage one probsat server with it's workers
 * example: ./manager test _no_output_file ../tests/testcluster < ../tests/demo2.cmd
 * or write help after starting without providing the demo.cmd to get a list of commands
 */

#include "config.hpp"
#include "util/util.hpp"
#include "communication/manager.hpp"

#include <thread>
#include <chrono>
#include <map>

// mpirun -host manager,manager -np 1 manager test
std::string server_name;
bool running = true;

typedef void (*command_function)(std::vector<std::string> &);
std::map<std::string, command_function> command_registry;

std::vector<MPI_Comm> workers;


void cmd_help(std::vector<std::string> &args) {
	std::ignore = args;
	std::cout << "commands available:" << std::endl;
	std::cout << "\thelp (shows this message)" << std::endl;
	std::cout << "\tadd_workers <num_workers> [hostfile] [worker arguments ...]" << std::endl;
	std::cout << "\t\t one may use >>use_no_hostfile<< as placeholder for the hostfile" << std::endl;
	std::cout << "\tadd_file <anzahl_startbelegungen> <anzahl_flips> formula.cnf" << std::endl;
	std::cout << "\twait_for_server" << std::endl;
	std::cout << "\texit" << std::endl;
}


void cmd_add_workers(std::vector<std::string> &args)
{
	if (solution_found) { return; }
	
	if (args.size() < 2) {
		throw std::runtime_error("manager: invalid number of arguments for add_workers");
	}

	int num_workers = std::stoi(args[1]);
	int num_workers_to_add = num_workers;
	
	MPI_Info info = MPI_INFO_NULL;
	if (3 <= args.size()) {
		if (args[2] != "use_no_hostfile") {
			MPI_Info_create(&info);
			MPI_Info_set(info, "add-hostfile", args[2].c_str());
		}
	}

	#if DEBUG_COMMUNICATION
		std::cout << "manager: adding " << num_workers << " workers" << std::endl;
	#endif

	// unperformant fix for OpenMPI MPI_Comm_Accept problems with multiple simultaneously connection attempts
	// if (0 < num_workers) {
	while (0 < num_workers_to_add) {
		std::vector<std::string> worker_argv = {server_name};
		for (std::size_t i = 3; i < args.size(); i++) {
			worker_argv.push_back(args[i]);
		}

		auto nw = std::min(MAX_SIMULTANEOUS_CONNECTION_ATTEMPTS, num_workers_to_add);
		MPI_Comm worker_cl = do_add_workers(nw, info, worker_argv);
		num_workers_to_add -= nw;
		// MPI_Comm worker_cl = do_add_workers(num_workers, info, worker_argv);

		assert(MPI_COMM_NULL != worker_cl);
		workers.push_back(worker_cl);
	}
	
	#if DEBUG_COMMUNICATION
		std::cout << "manager: " << num_workers << " workers added" << std::endl;
	#endif
}


void cmd_add_file(std::vector<std::string> &args)
{
	if (solution_found) { return; }

	if (4 != args.size()) {
		throw std::runtime_error("manager: invalid number of arguments for add_file");
	}

	do_add_file(std::stoi(args[1]), std::stoi(args[2]), args[3]);
}


void cmd_wait_for_server(std::vector<std::string> &args) {
	if (1 != args.size()) {
		throw std::runtime_error("manager: invalid number of arguments for wait_for_server");
	}

	MPI_Status status;
	do {
		command_buffer.clear();
		MPI_Recv(command_buffer.data(), command_buffer.capacity(), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, server_cl, &status);
		process_server_cmd(status);
	} while (status.MPI_TAG != S2M::DISCONNECT);
	do_exit();
	running = false;
}

void cmd_exit(std::vector<std::string> &args) {
	std::ignore = args;
	do_exit();
	running = false;
}

int main(int argc, char *argv[])
{
	auto start = std::chrono::high_resolution_clock::now();
	decltype(start) start_running;
	decltype(start) stop_running;
	decltype(std::chrono::duration_cast<std::chrono::microseconds>(start_running-stop_running)) running_duration;

	try {
		if ((argc < 2) || (argc > 4)) {
			throw std::runtime_error("name for problem instance and optional an output filename (or _no_output_file) as well as mpi hostfile is required!");
		}

		std::string output_filename = "statistic-output.txt";

		server_name = argv[1];
		char sn_str[server_name.length() + 1];
		strcpy(sn_str, server_name.c_str());
		sn_str[server_name.length()] = 0;
		char *server_argv[2] = {sn_str, nullptr}; // MPI_ARGV_NULL
		
		const char *hostfile = nullptr;
		if (3 <= argc) {
			output_filename = std::string(argv[2]);
		}

		if (4 <= argc) {
			hostfile = argv[3];
		}

		output_to_file = (std::string("_no_output_file") != output_filename);
		if (output_to_file) {
			output_file.open(output_filename, std::ios::out);
		}

		command_registry["help"] = &cmd_help;
		command_registry["add_workers"] = &cmd_add_workers;
		command_registry["add_file"] = &cmd_add_file;
		command_registry["wait_for_server"] = &cmd_wait_for_server;
		command_registry["exit"] = &cmd_exit;

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

		int manager_id = 0;
		MPI_Comm_rank(MPI_COMM_WORLD, &manager_id);
		if (0 != manager_id) {
			throw std::runtime_error("manager: got id " + std::to_string(manager_id) + " > 0, but you probably want only one manager...");
		}

		int length = 0;
		char processor_name[MPI_MAX_PROCESSOR_NAME];
		MPI_Get_processor_name(processor_name, &length);
		#if DEBUG_COMMUNICATION
			std::cout << msg_header << "running on " << processor_name << std::endl;
		#endif

		MPI_Info info = MPI_INFO_NULL;
		if (nullptr != hostfile) {
			MPI_Info_create(&info);
			MPI_Info_set(info, "add-hostfile", hostfile);
		}

		MPI_Comm_spawn("server", server_argv, 1, info, 0, MPI_COMM_SELF, &server_cl, MPI_ERRCODES_IGNORE);
		assert(0 == sn_str[server_name.length()]);

		#if DEBUG_COMMUNICATION
			std::cout << msg_header << "executing ./server";
			for (std::size_t i = 0; server_argv[i]; i++) { std::cout << " " << server_argv[i]; }
			std::cout << std::endl;
		#endif

		std::deque<std::string> default_cmds = {};
			// "add_file 2 50 ../tests/k3-n60-m256-r4.267-s30730906_mod_s1750274_p8.0093584347461e-06.cnf",
			// "add_file 1 50 ../tests/gen_n50_m213_k3SAT_seed1511168162_mod_s257282_p8.44605646486143e-06.cnf",
			// "add_file 4 100 ../tests/k3-n60-m256-r4.267-s30730906_mod_s1750274_p8.0093584347461e-06.cnf",
			// "add_file 2 100 ../tests/gen_n50_m213_k3SAT_seed1511168162_mod_s257282_p8.44605646486143e-06.cnf",
			// "add_file 3 1000 ../tests/gen_n50_m213_k3SAT_seed1511168162_mod_s257282_p8.44605646486143e-06.cnf",
			// "add_workers 2",
			// "wait_for_server",
			// "exit"

		std::cin.clear();
		// std::cin.seekg(0);

		start_running = std::chrono::high_resolution_clock::now();
		while (running) {
			std::string line;
			if (default_cmds.empty()) {
				if (std::cin) {
					while (line.empty()) {
						if (!std::getline(std::cin, line)) {
							static bool once = true;
							if (once) {
								std::cin.clear();
								std::cin.seekg(0);
								once = false;
							}
						}
					}
				} else {
					default_cmds.push_front("exit");
				}
			} else {
				line = default_cmds.front();
				default_cmds.pop_front();
			}

			std::vector<std::string> fields;
			split(line, fields);

			auto cmd_func_it = command_registry.find(fields[0]);
			if (command_registry.end() == cmd_func_it) {
				std::cerr << "invalid command '" << fields[0] << "' called" << std::endl;
			} else {
				try {
					#if DEBUG_COMMUNICATION
						std::cout << "executing command " << fields[0] << std::endl;
					#endif
					auto cmd_func = *cmd_func_it->second;
					assert(cmd_func);
					cmd_func(fields);
				} catch (const std::exception &e) {
					std::cerr << "error executing command '" << fields[0] << "': " << e.what() << std::endl;
				}
			}
			
			MPI_Status status;
			int flag = 0;
			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, server_cl, &flag, &status);
			if (flag) {
				if (status.MPI_TAG == S2M::FOUND_SOLUTION) {
					default_cmds.push_front("wait_for_server");
				} else if (status.MPI_TAG == S2M::DISCONNECT) {
					default_cmds.push_front("exit");
				}

				command_buffer.clear();
				MPI_Recv(command_buffer.data(), command_buffer.capacity(), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, server_cl, &status);
				process_server_cmd(status);
			}
		}
		stop_running = std::chrono::high_resolution_clock::now();

		outp_stat(server_name << " overall statistic:")
		outp_stat(statistic.num_instances_started << " instances started")
		outp_stat(statistic.total_num_flips << " flips executed")
		outp_stat(statistic.avg_flips_per_second << " flips per second (avg)")
		outp_stat(statistic.avg_flips_per_var << " flips per var (avg)")
		outp_stat(statistic.avg_flips_per_clause << " flips per clause (avg)")
		outp_stat(statistic.total_init_duration << " total init duration (ms)")
		outp_stat(statistic.total_solve_duration << " total solve duration (ms)")
		outp_stat(statistic.total_overall_duration << " total overall duration (ms)")
		outp_stat(statistic.num_instances_solved << " instances solved")
		if (0 < statistic.num_instances_solved) {
			outp_stat(statistic.min_flips_to_solve << " flips until solved (min)")
		}
		if (output_to_file) { output_file << std::endl; }
		if (output_to_stdout) { std::cout << std::endl; }


		running_duration = std::chrono::duration_cast<std::chrono::microseconds>(stop_running - start_running);
		
		outp_stat(msg_header << "running duration: " << (running_duration.count() / 1000.) << " ms")

	} catch (const std::exception& ex) {
		print_exception("manager", ex);
	}

	try {
		auto done = std::chrono::high_resolution_clock::now();
		auto overall_duration = std::chrono::duration_cast<std::chrono::microseconds>(done - start);
		auto manager_overhead = overall_duration - running_duration;

		outp_stat(msg_header << "manager overhead: " << (manager_overhead.count() / 1000.) << " ms")
		outp_stat(msg_header << "overall duration: " << (overall_duration.count() / 1000.) << " ms")

		if ((output_to_file) && (output_file)) {
			output_file.close();
		}

		{
			MPI_Barrier(server_cl);
			MPI_Comm_disconnect(&server_cl);

			MPI_Comm last = MPI_COMM_NULL;
			for (std::size_t i = 0; i < workers.size(); i++) {
				if (last != workers[i]) {
					last = workers[i];
					MPI_Barrier(workers[i]);
					MPI_Comm_disconnect(&workers[i]);
				}
			}

			int is_initialized;
			MPI_Initialized(&is_initialized);
			assert(is_initialized);
			MPI_Barrier(MPI_COMM_WORLD);
			MPI_T_finalize();
		}

		// using namespace std::chrono_literals;
		// std::this_thread::sleep_for(50ms);
	} catch (const std::exception& ex) {
		print_exception("manager", ex);
	}

	#if DEBUG_COMMUNICATION
		std::cout << "manager finished" << std::endl;
	#endif

	return EXIT_SUCCESS;
}
