#ifndef MANAGER_HPP
#define MANAGER_HPP

// Programmcode zur Kommunikation für den Server

#define BUILD_SERVER 1
#include "cmd/ws_typedefs.hpp"
#include "manager_server.hpp"
#include "server_worker.hpp"

#include <vector>
#include <deque>
#include <string.h>
#include <cmath>


struct task_info {
	problem_instance_metadata metadata;
	problem_instance_statistics statistics;
	uint32_t num_currently_processing;
	bool is_scheduled;

	task_info(problem_instance_metadata data) : metadata(data), statistics(), num_currently_processing(0), is_scheduled(true) {}
};

std::vector<task_info> available_tasks;
std::deque<uint32_t> task_order;
std::vector<MPI_Request> work_unit_requests;


const char *server_name = nullptr;
MPI_Info server_info = MPI_INFO_NULL;
char port_name[MPI_MAX_PORT_NAME];


// if false tasks with anzahl_startbelegungen = 0 will be probed for ever
constexpr bool try_once = true;
bool wait_for_more_files = false;
bool terminate_after_solution_was_found = true;
bool short_statistic = true;

// current program state
bool running = true;
bool solution_found = false;
bool terminated_workers = false;
bool statistic_send = false;


// how many instances are currently at workers
uint64_t num_instances_in_processing = 0;


void process_m2s_ADD_FILE(MPI_Status &status) {
	auto af_cmd = M2S::add_file();
	af_cmd.process(status);

	#if DEBUG_COMMUNICATION
		std::cout << msg_header << "server added file " << af_cmd.data.filename << " with "
			<< af_cmd.data.anzahl_startbelegungen << " initializations and " << af_cmd.data.anzahl_flips << " flips" << std::endl;
	#endif

	bool is_first_file = available_tasks.empty();

	available_tasks.push_back(task_info(af_cmd.data));
	task_order.push_front(available_tasks.size()-1);

	if ((false == work_units.empty()) && is_first_file) {
		for (std::size_t i = 0; i < work_units.size(); i++) {
			S2W::activate_worker::send(i, 0);
			
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "activated worker " << (i+1) << "/" << work_units.size() << std::endl;
			#endif
		}
	}
}


void process_m2s_ADD_WORKERS(MPI_Status &status) {
	int num_workers = M2S::add_workers::process(status);
	if (0 >= num_workers) {
		throw std::runtime_error(std::string(msg_header) + "invalid ADD_WORKER command: must add at least one "
			+ "worker (requested were " + std::to_string(num_workers) + ")");
	}

	MPI_Barrier(manager_cl);

	std::size_t offset = work_units.size();
	for (int i = 0; i < num_workers; i++) {
		MPI_Comm worker;
		assert(MPI_SUCCESS == MPI_Comm_accept(port_name, server_info, 0, MPI_COMM_SELF, &worker));
		// assert(MPI_SUCCESS == MPI_Comm_accept(server_name, server_info, MPI_ANY_SOURCE, MPI_COMM_WORLD, &worker));
		work_units.push_back(worker);

		#if DEBUG_COMMUNICATION
			std::cout << msg_header << "accepted worker " << (i+1) << "/" << num_workers << std::endl;
		#endif
	}

	for (int i = 0; i < num_workers; i++) {
		transfer_sizes.emplace_back(default_max_cmd_length);
		worker_comm_buffers.emplace_back(default_max_cmd_length);

		MPI_Request request;
		MPI_Irecv(worker_comm_buffers.back().data(), worker_comm_buffers.back().capacity(), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, work_units[offset + i], &request);
		work_unit_requests.push_back(request);

		#if DEBUG_COMMUNICATION
			std::cout << msg_header << "startet request for worker " << (i+1) << "/" << num_workers << std::endl;
		#endif
	}

	if (false == available_tasks.empty()) {
		for (int i = 0; i < num_workers; i++) {
			S2W::activate_worker::send(offset + i, 0);
			
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "activated worker " << (i+1) << "/" << num_workers << std::endl;
			#endif
		}
	}

	MPI_Barrier(manager_cl);
}


void send_termination_to_workers() {
	if (terminated_workers) { return; }
	terminated_workers = true;

	#if DEBUG_COMMUNICATION
		std::cout << msg_header << "sending TERMINATE to all workers" << std::endl;
	#endif

	MPI_Comm curr_cl;
	assert(0 < work_units.size());
	std::size_t i = 0;
	do {
		curr_cl = work_units[i];

		int comm_size;
		MPI_Comm_size(curr_cl, &comm_size);

		for (int id = 0; id < comm_size; id++) {
			S2W::terminate::send(i, id);
		}

		do {i++;} while (curr_cl == work_units[i]);
	} while (i < work_units.size());
}


void process_manager_cmd(MPI_Status &status) {
	int command = status.MPI_TAG;
	
	#if DEBUG_COMMUNICATION
		std::cout << msg_header << "process_manager_cmd waiting for manager command " << command << std::endl;
	#endif
	
	switch (command) {
		case M2S::REQ_MAX_CMD_LENGTH:
			command_buffer.clear();
			MPI_Recv(command_buffer.data(), command_buffer.capacity(), MPI_BYTE, MPI_ANY_SOURCE, M2S::REQ_MAX_CMD_LENGTH, manager_cl, &status);
			M2S::req_max_transfer_size::process(status, command_buffer);
			break;

		case M2S::ADD_WORKERS:
			process_m2s_ADD_WORKERS(status);
			break;

		case M2S::ADD_FILE:
			process_m2s_ADD_FILE(status);
			break;

		case M2S::TERMINATE:
			M2S::terminate::process(status);
			send_termination_to_workers();
			// running = false;
			break;

		default:
			throw std::runtime_error(std::string(msg_header) + "unknown command (M2S:: " + std::to_string(command) + ")");
	}
}


void process_w2s_REQ_MAX_CMD_LENGTH(int index, MPI_Status &status)
{
	// assert(MPI_SUCCESS == MPI_Request_free(&work_unit_requests[index]));
	work_unit_requests[index] = nullptr;
	W2S::req_max_transfer_size::process(status, worker_comm_buffers[index]);
	// MPI_Irecv(worker_comm_buffers[index].data(), worker_comm_buffers[index].capacity(),
		// MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, work_units[index], &work_unit_requests[index]);
}


void process_w2s_FOUND_SOLUTION(int index, MPI_Status &status) {
	W2S::found_solution fs_cmd;
	fs_cmd.process(index, status);
	solution_found = true;
	
	// server will print statistics
	// std::cout << std::endl;
	// std::cout << "c SATISFIABLE" << std::endl;
	// std::cout << "c filename: " << fs_cmd.info.filename << std::endl;
	// std::cout << "c seed: " << fs_cmd.info.seed << std::endl;
	// std::cout << "c num_flips_done: " << fs_cmd.info.num_flips_done << std::endl;
	// std::cout << "c solve_duration: " << (fs_cmd.info.solve_duration / 1000.) << " ms" << std::endl;
	// std::cout << "c num_vars: " << fs_cmd.info.num_vars << std::endl;
	// std::cout << "c num_clauses: " << fs_cmd.info.num_clauses << std::endl;
	// std::cout << "c flips per second: " << round((double) fs_cmd.info.num_flips_done / fs_cmd.info.solve_duration * 1000000.) << std::endl;
	// std::cout << "c flips per var: " << ((double) fs_cmd.info.num_flips_done / fs_cmd.info.num_vars) << std::endl;
	// std::cout << "c flips per clause: " << ((double) fs_cmd.info.num_flips_done / fs_cmd.info.num_clauses) << std::endl;
	// std::cout << std::endl;

	if (terminate_after_solution_was_found) {
		send_termination_to_workers();
	}

	S2M::found_solution fs_mng_cmd(fs_cmd.info);
	fs_mng_cmd.send();
}


void send_statistic_to_manager()
{
	if (statistic_send) return;
	statistic_send = true;

	for (auto t : available_tasks) {
		// std::cout << "INFO: " << t.metadata.filename << std::endl;
		if ((0 < t.statistics.num_instances_started) || (!short_statistic)) {
			S2M::send_statistics scmd = S2M::send_statistics({t.metadata, t.statistics});
			scmd.send();
		}
	}

	// S2M::disconnect::send();
}


void process_w2s_DONE_PROCESSING(int index, MPI_Status &status)
{
	auto dp_cmd = W2S::done_processing();
	dp_cmd.process(index, status);
	auto msg = dp_cmd.info;

	problem_instance_statistics &s = available_tasks[msg.task_id].statistics;

	if (0 < msg.num_flips_done) {
		if (0 < s.total_num_flips) {
			assert(s.num_vars == msg.num_vars);
			assert(s.num_clauses == msg.num_clauses);
		} else {
			s.num_vars = msg.num_vars;
			s.num_clauses = msg.num_clauses;
		}
	}
	
	s.total_num_flips += msg.num_flips_done;
	s.avg_flips_per_second = (s.avg_flips_per_second * s.num_instances_started + msg.flips_per_second * msg.instances_processed)
		/ (s.num_instances_started + msg.instances_processed);
	s.num_instances_started += msg.instances_processed;
	s.total_init_duration += msg.init_duration;
	s.total_solve_duration += msg.solve_duration;
	s.total_overall_duration += msg.overall_duration;

	if (msg.solved) {
		s.times_solved++;
		s.min_flips_to_solve = std::min(s.min_flips_to_solve, msg.num_flips_done);
	}

	assert(num_instances_in_processing >= msg.instances_processed);
	num_instances_in_processing -= msg.instances_processed;

	available_tasks[msg.task_id].num_currently_processing -= msg.instances_processed;

	if (false == available_tasks[msg.task_id].is_scheduled) {
		const uint32_t n = available_tasks[msg.task_id].metadata.anzahl_startbelegungen;
		const uint32_t cp = available_tasks[msg.task_id].num_currently_processing;
		// if (((false == try_once) && (0 == n)) || (n > s.num_instances_started)) {
		if (((false == try_once) && (0 == n)) || (n > s.num_instances_started + cp)) {
			// std::cout << n << " > " << s.num_instances_started << std::endl;
			task_order.push_back(msg.task_id); // reschedule
			available_tasks[msg.task_id].is_scheduled = true;
		}
	}
	
	// std::cout << "process_w2s_DONE_PROCESSING:" << std::endl;
	// std::cout << "num_instances_in_processing: " << num_instances_in_processing << std::endl;
	// std::cout << "task_order.empty(): " << task_order.empty() << std::endl;
	// std::cout << "solution_found: " << solution_found << std::endl;
	if ((task_order.empty() || (solution_found && terminate_after_solution_was_found)) && (0 == num_instances_in_processing)) {
	// if ((task_order.empty()) && (0 == num_instances_in_processing)) {
		// all requests were returned
		#if DEBUG_COMMUNICATION
			std::cout << msg_header << "all requests were returned, send statistics for " << available_tasks.size() << " tasks!" << std::endl;
		#endif
		
		if (!wait_for_more_files) {
			send_statistic_to_manager();
			send_termination_to_workers();
		}
	}
}

void process_w2s_DISCONNECT(int index, MPI_Status &status) {
	#if DEBUG_COMMUNICATION
		std::cout << msg_header << "worker " << index << " disconnected" << std::endl;
	#endif

	W2S::disconnect::process(index, status);
	// assert(MPI_SUCCESS == MPI_Request_free(&work_unit_requests[index]));
	MPI_Barrier(work_units[index]);
	MPI_Comm_disconnect(&work_units[index]);
	assert(MPI_COMM_NULL == work_units[index]);

	work_unit_requests.erase(work_unit_requests.begin() + index);
	work_units.erase(work_units.begin() + index);
	transfer_sizes.erase(transfer_sizes.begin() + index);
	worker_comm_buffers.erase(worker_comm_buffers.begin() + index);

	work_units_disconnected++;

	// std::cout << "process_w2s_DISCONNECT:" << std::endl;
	// std::cout << "num_instances_in_processing: " << num_instances_in_processing << std::endl;
	// std::cout << "task_order.empty(): " << task_order.empty() << std::endl;
	// std::cout << "solution_found: " << solution_found << std::endl;
	// std::cout << "work_units.empty(): " << work_units.empty() << std::endl;
	// std::cout << "statistic_send: " << statistic_send << std::endl;
	// if ((task_order.empty() || solution_found) && (work_units_disconnected == work_units.size())) {
	if ((task_order.empty() || solution_found) && work_units.empty() && statistic_send) {
		send_statistic_to_manager();
		running = false;
	}
}


void process_w2s_GET_INSTANCES(int index, MPI_Status &status) {
	W2S::get_instances gi_req;
	gi_req.process(index, status);
	auto request = gi_req.info;

	S2W::send_instances si_rep;
	std::vector<job> &reply = si_rep.jobs;
	if (false == solution_found) {
		while (0 < request.num_instances_requested) {
			if (task_order.empty()) { break; }
			
			const uint32_t task_id = task_order.front();
			const uint32_t num_instances_required = available_tasks[task_id].metadata.anzahl_startbelegungen;
			const uint32_t num_processed = try_once ? available_tasks[task_id].statistics.num_instances_started : 0;
			const uint32_t instances_available = (0 == num_instances_required) ? request.num_instances_requested
				: num_instances_required - available_tasks[task_id].num_currently_processing - num_processed;

			const uint32_t instances_to_start = std::min(request.num_instances_requested, instances_available);
			assert(0 < instances_to_start);

			job j = {available_tasks[task_id].metadata, instances_to_start, task_id};
			reply.push_back(j);

			available_tasks[task_id].num_currently_processing += instances_to_start;

			// std::cout << instances_available << " <= " << instances_to_start << std::endl;
			if (instances_available <= instances_to_start) {
				task_order.pop_front();
				available_tasks[task_id].is_scheduled = false;
			}
			
			// std::cout << "num_instances_required: " << num_instances_required <<
				// ", instances_available: " << instances_available << std::endl;
			assert(request.num_instances_requested >= instances_to_start);
			request.num_instances_requested -= instances_to_start;
			num_instances_in_processing += instances_to_start;
		}
	}
	
	si_rep.send(index, status.MPI_SOURCE);
}


void process_worker_cmd(int index, MPI_Status &status) {
	// MPI_Status status;
	// MPI_Wait(&work_unit_requests[index], &status);

	#if DEBUG_COMMUNICATION
		std::cout << msg_header << "process_worker_cmd recieved worker command "
			<< status.MPI_TAG << " by worker " << index << " (" << status.MPI_SOURCE << ")" << std::endl;
	#endif

	switch (status.MPI_TAG) {
		case W2S::REQ_MAX_CMD_LENGTH:
			process_w2s_REQ_MAX_CMD_LENGTH(index, status);
			break;

		case W2S::GET_INSTANCES:
			process_w2s_GET_INSTANCES(index, status);
			break;

		case W2S::DONE_PROCESSING:
			process_w2s_DONE_PROCESSING(index, status);
			break;

		case W2S::FOUND_SOLUTION:
			process_w2s_FOUND_SOLUTION(index, status);
			break;

		case W2S::DISCONNECT:
			process_w2s_DISCONNECT(index, status);
			return;

		default:
			throw std::runtime_error("unknown command (W2S:: " + std::to_string(status.MPI_TAG) + ")");
	}

	// renew request
	// assert(MPI_SUCCESS == MPI_Request_free(&work_unit_requests[index]));
	MPI_Irecv(worker_comm_buffers[index].data(), worker_comm_buffers[index].capacity(),
		MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, work_units[index], &work_unit_requests[index]);
}

#endif
