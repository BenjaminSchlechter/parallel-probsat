#ifndef WORKER_HPP
#define WORKER_HPP

// Programmcode zur Kommunikation für den Worker

#define ENALBE_CNF_MULTITHREAD_SHARING 1

#define BUILD_WORKER 1
#include "cmd/ws_typedefs.hpp"
#include "server_worker.hpp"
#include "../worker_task.hpp"


void process_s2w_send_instances(MPI_Status &status) {
	S2W::send_instances si_cmd;
	si_cmd.process(status);

	if (!running) { return; }

	if (!si_cmd.jobs.empty()) {
		create_tasks(si_cmd.jobs);
	}
}


void process_server_cmd(MPI_Status &status) {
	#if DEBUG_COMMUNICATION
		std::cout << msg_header << "processing server command " << status.MPI_TAG << std::endl;
	#endif

	switch (status.MPI_TAG) {
		case S2W::REQ_MAX_CMD_LENGTH:
			S2W::req_max_transfer_size::process(status, command_buffer);
			break;

		case S2W::ACTIVATE_WORKER:
			S2W::activate_worker::process(status);
			break;
			
		case S2W::SEND_INSTANCES:
			process_s2w_send_instances(status);
			break;

		case S2W::TERMINATE:
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "recieved TERMINATE command (" << status.MPI_TAG << ") from " << status.MPI_SOURCE << std::endl;
			#endif
			running = false;
			break;

		default:
			throw std::runtime_error(msg_header + std::string("unknown command (S2W:: ") + std::to_string(status.MPI_TAG) + std::string(")"));
	}
}


void recieve_server_cmd(const S2W::CTAG_S2W wait_for = S2W::INVALID_CMD) {
	MPI_Status status;
	do {
		command_buffer.clear();
		MPI_Recv(command_buffer.data(), command_buffer.capacity(), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, server_cl, &status);
		#if DEBUG_COMMUNICATION
			std::cout << msg_header << "recieve_server_cmd got command " << status.MPI_TAG << std::endl;
		#endif
		process_server_cmd(status);
	} while ((wait_for != S2W::INVALID_CMD) && (wait_for != status.MPI_TAG));
}

#endif
