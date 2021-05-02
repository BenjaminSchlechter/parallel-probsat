#ifndef WS_TYPEDEFS_HPP
#define WS_TYPEDEFS_HPP

// gemeinsame Typdefinitionen die bei der Kommunikation verwendet werden

#include "../common.hpp"


struct problem_instance_metadata {
	// Anzahl der Startbelegungen die für diese Datei getestet werden sollen (falls 0 nutze beliebig oft die Einstellung des Workers)
	uint32_t anzahl_startbelegungen = 0;
	// Anzahl der Flips die pro Startbelegung durchgeführt werden (falls 0 nutze Einstellung des Workers)
	uint32_t anzahl_flips = 0;
	// Dateiname der von jedem Worker erreichbar ist
	std::string filename;
};

template <typename S>
void serialize (S& s, problem_instance_metadata& o) {
	s.value4b(o.anzahl_startbelegungen);
	s.value4b(o.anzahl_flips);
	s.text1b(o.filename, absolute_max_cmd_length - 8);
}


struct done_processing_info {
	uint32_t task_id = 0;
	uint32_t instances_processed = 0;
	uint64_t num_flips_done = 0;
	double flips_per_second = 0.0;
	uint64_t init_duration = 0;
	uint64_t solve_duration = 0;
	uint64_t overall_duration = 0;
	uint64_t seed = 0;
	uint64_t num_vars = 0;
	uint64_t num_clauses = 0;
	bool solved = false;
};

template <typename S>
void serialize (S& s, done_processing_info& o) {
	s.value4b(o.task_id);
	s.value4b(o.instances_processed);
	s.value8b(o.num_flips_done);
	s.value8b(o.flips_per_second);
	s.value8b(o.init_duration);
	s.value8b(o.solve_duration);
	s.value8b(o.overall_duration);
	s.value8b(o.seed);
	s.value8b(o.num_vars);
	s.value8b(o.num_clauses);
	s.value1b(o.solved);
}


struct problem_instance_statistics {
	uint64_t num_instances_started = 0;
	uint64_t total_num_flips = 0;
	double avg_flips_per_second = 0.;
	uint64_t total_init_duration = 0;
	uint64_t total_solve_duration = 0;
	uint64_t total_overall_duration = 0;
	uint64_t num_vars = 0;
	uint64_t num_clauses = 0;
	uint64_t times_solved = 0;
	uint64_t min_flips_to_solve = std::numeric_limits<uint64_t>::max();
};

template <typename S>
void serialize (S& s, problem_instance_statistics& o) {
	s.value8b(o.num_instances_started);
	s.value8b(o.total_num_flips);
	s.value8b(o.avg_flips_per_second);
	s.value8b(o.total_init_duration);
	s.value8b(o.total_solve_duration);
	s.value8b(o.total_overall_duration);
	s.value8b(o.num_vars);
	s.value8b(o.num_clauses);
	s.value8b(o.times_solved);
	s.value8b(o.min_flips_to_solve);
}


struct job {
	problem_instance_metadata metadata;
	uint32_t num_instances_to_start;
	uint32_t task_id;
};

template <typename S>
void serialize (S& s, job& o) {
	serialize(s, o.metadata);
	s.value4b(o.num_instances_to_start);
	s.value4b(o.task_id);
}

template <typename S>
void serialize (S& s, std::vector<job>& o) {
	s.container(o, absolute_max_cmd_length);
}


struct solution_info {
	uint64_t seed = 0;
	uint64_t num_flips_done = 0;
	uint64_t solve_duration = 0;
	uint64_t num_vars = 0;
	uint64_t num_clauses = 0;
	std::string filename;
};

template <typename S>
void serialize (S& s, solution_info& o) {
	s.value8b(o.seed);
	s.value8b(o.num_flips_done);
	s.value8b(o.solve_duration);
	s.value8b(o.num_vars);
	s.value8b(o.num_clauses);
	s.text1b(o.filename, absolute_max_cmd_length - 16);
}


#endif
