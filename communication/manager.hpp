#ifndef MANAGER_HPP
#define MANAGER_HPP

// Programmcode zur Kommunikation für den Manager

#define BUILD_MANAGER 1
#include "manager_server.hpp"

#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>

// required for strcpy_s
#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>

#ifndef __STDC_LIB_EXT1__
	// static_assert(false && "strcpy_s is not available");
	int strcpy_s(char *dest, std::size_t buflen, const char *src) {
		std::ignore = buflen;
		strcpy(dest, src);
		return 0;
	}
#endif


bool solution_found = false;


MPI_Comm do_add_workers(const int num_workers, MPI_Info &info, const std::vector<std::string> worker_argv)
{
	if (0 >= num_workers) {
		return MPI_COMM_NULL;
	}

	M2S::add_workers::send(num_workers);

	MPI_Comm worker_cl;

	auto worker_argv_ = MPI_ARGV_NULL;
	char *argv_[worker_argv.size()+1];
	argv_[worker_argv.size()] = nullptr;
	std::size_t len = 0;

	if (0 < worker_argv.size()) {
		for (auto arg : worker_argv) {
			// std::cout << "arg: " << arg << std::endl;
			len += arg.length()+1;
		}
	}

	char strbuf[len];
	if (0 < worker_argv.size()) {
		std::size_t offset = 0;
		std::size_t i = 0;
		for (auto arg : worker_argv) {
			assert(0 == strcpy_s(&strbuf[offset], arg.length(), arg.c_str()));
			strbuf[offset+arg.length()] = 0;
			argv_[i] = &strbuf[offset];
			offset += arg.length() + 1;
			i++;
		}
		
		worker_argv_ = argv_;
	}

	MPI_Barrier(server_cl);

	assert(MPI_SUCCESS == MPI_Comm_spawn("worker", worker_argv_, num_workers, info, 0, MPI_COMM_SELF, &worker_cl, MPI_ERRCODES_IGNORE));

	#if DEBUG_COMMUNICATION
		std::cout << msg_header << "executing " << std::to_string(num_workers) << " times ./worker";
		for (std::size_t i = 0; worker_argv_[i]; i++) { std::cout << " " << worker_argv_[i]; }
		std::cout << std::endl;
	#endif

	MPI_Barrier(server_cl);

	return worker_cl;
}


void do_add_file(const uint32_t anzahl_startbelegungen, const uint32_t anzahl_flips, const std::string filename)
{
	auto af_cmd = M2S::add_file({anzahl_startbelegungen, anzahl_flips, filename});
	af_cmd.send();
}


void do_exit()
{
	M2S::terminate::send();
}


struct complete_statistic {
	std::size_t num_instances_started = 0;
	std::size_t total_num_flips = 0;
	double avg_flips_per_second = 0.0;
	double avg_flips_per_var = 0.0;
	double avg_flips_per_clause = 0.0;
	// durations in ms!
	double total_init_duration = 0.;
	double total_solve_duration = 0.;
	double total_overall_duration = 0.;
	std::size_t num_instances_solved = 0;
	std::size_t min_flips_to_solve = std::numeric_limits<std::size_t>::max();
};

complete_statistic statistic;

bool output_to_file = false;
std::ofstream output_file;

#if OUTPUT_STATISTIC
	const bool output_to_stdout = true;
#else
	const bool output_to_stdout = false;
#endif

#define outp_stat(line) \
	if (output_to_stdout) { std::cout << "c " << line << std::endl; } \
	if (output_to_file) { output_file << line << std::endl; }


void process_s2m_SEND_STATISTICS(MPI_Status &status)
{
	S2M::send_statistics scmd;
	scmd.process(status);
	auto mdat = scmd.data.metadata;
	auto stat = scmd.data.statistics;

	double avg_flips_per_var = ((double) stat.total_num_flips / stat.num_vars);
	double avg_flips_per_clause = ((double) stat.total_num_flips / stat.num_clauses);

	auto inst_started = statistic.num_instances_started + stat.num_instances_started;
	statistic.avg_flips_per_second = (statistic.avg_flips_per_second * statistic.num_instances_started
		+ stat.avg_flips_per_second * stat.num_instances_started) / inst_started;
	statistic.avg_flips_per_var = (statistic.avg_flips_per_var * statistic.num_instances_started
		+ avg_flips_per_var * stat.num_instances_started) / inst_started;
	statistic.avg_flips_per_clause = (statistic.avg_flips_per_clause * statistic.num_instances_started
		+ avg_flips_per_clause * stat.num_instances_started) / inst_started;

	statistic.num_instances_started = inst_started;
	statistic.total_num_flips += stat.total_num_flips;
	statistic.total_init_duration += stat.total_init_duration / 1000.;
	statistic.total_solve_duration += stat.total_solve_duration / 1000.;
	statistic.total_overall_duration += stat.total_overall_duration / 1000.;
	statistic.num_instances_solved += stat.times_solved;
	// if (0 < stat.times_solved) {
		// statistic.min_flips_to_solve = std::min(statistic.min_flips_to_solve, stat.total_num_flips);
	// }

	#if OUTPUT_STATISTIC
		outp_stat("filename: " << mdat.filename)
		outp_stat(stat.num_vars << " num variables")
		outp_stat(stat.num_clauses << " num clauses")
		outp_stat(stat.num_instances_started << " / " << mdat.anzahl_startbelegungen << " startbelegungen")
		auto prod_flips_startbelegungen = mdat.anzahl_flips * mdat.anzahl_startbelegungen;
		outp_stat(stat.total_num_flips << " / (" << mdat.anzahl_flips << " * " << mdat.anzahl_startbelegungen
			<< " = " << prod_flips_startbelegungen << ") flips")
		outp_stat(stat.avg_flips_per_second << " flips per second (avg)")
		outp_stat(avg_flips_per_var << " flips per var (avg)")
		outp_stat(avg_flips_per_clause << " flips per clause (avg)")
		outp_stat(stat.total_init_duration << " total init duration (us)")
		outp_stat(stat.total_solve_duration << " total solve duration (us)")
		outp_stat(stat.total_overall_duration << " total overall duration (us)")
		outp_stat(stat.times_solved << " times solved")
		outp_stat(stat.min_flips_to_solve << " flips until solved (min)")
		if (output_to_file) { output_file << std::endl; }
		if (output_to_stdout) { std::cout << std::endl; }
	#endif
}


void process_s2m_FOUND_SOLUTION(MPI_Status &status) {
	S2M::found_solution fs_cmd;
	fs_cmd.process(status);

	solution_found = true;

	statistic.min_flips_to_solve = std::min(statistic.min_flips_to_solve, fs_cmd.info.num_flips_done);

	outp_stat("SATISFIABLE")
	outp_stat("filename: " << fs_cmd.info.filename)
	outp_stat("seed: " << fs_cmd.info.seed)
	outp_stat("num_flips_done: " << fs_cmd.info.num_flips_done)
	auto solve_duration_ms = (fs_cmd.info.solve_duration / 1000.);
	outp_stat("solve_duration: " << solve_duration_ms << " ms")
	outp_stat("num_vars: " << fs_cmd.info.num_vars)
	outp_stat("num_clauses: " << fs_cmd.info.num_clauses)
	auto fps = round((double) fs_cmd.info.num_flips_done / fs_cmd.info.solve_duration * 1000000.);
	outp_stat("flips per second: " << fps)
	auto fpv = ((double) fs_cmd.info.num_flips_done / fs_cmd.info.num_vars);
	outp_stat("flips per var: " << fpv)
	auto fpc = ((double) fs_cmd.info.num_flips_done / fs_cmd.info.num_clauses);
	outp_stat("flips per clause: " << fpc)
	if (output_to_file) { output_file << std::endl; }
	if (output_to_stdout) { std::cout << std::endl; }
}


void process_server_cmd(MPI_Status &status)
{
	#if DEBUG_COMMUNICATION
		std::cout << msg_header << "process_server_cmd recieved server command "
			<< status.MPI_TAG << " from " << status.MPI_SOURCE << std::endl;
	#endif

	switch (status.MPI_TAG) {
		case S2M::FOUND_SOLUTION:
			process_s2m_FOUND_SOLUTION(status);
			break;

		case S2M::SEND_STATISTICS:
			process_s2m_SEND_STATISTICS(status);
			break;

		case S2M::DISCONNECT:
			S2M::disconnect::process(status);
			break;

		default:
			throw std::runtime_error("unknown command (S2M:: " + std::to_string(status.MPI_TAG) + ")");
	}
}


#endif
