#ifndef WORKER_TASK_HPP
#define WORKER_TASK_HPP

// Programmcode zum Managment der ProbSAT Threads und Programmcode dieser

#include "config.hpp"

#include "util/util.hpp"
#include "sat/3sat-clause.hpp"
#include "sat/instance.hpp"
#include "sat/probability_functions/polynomial.hpp"
#include "sat/probability_functions/exponential.hpp"
#include "sat/probability_functions/cached.hpp"

// #include "communication/worker.hpp"
#include "communication/cmd/ws_typedefs.hpp"

#include <chrono>
#include <cmath>
#include <memory>
#include <thread>
#include <condition_variable>


volatile bool running = true;

auto sleep_duration_between_get_task_polls = std::chrono::milliseconds(10);

std::size_t default_max_flips = 20000000;

// algorithm options:
constexpr bool caching = true;

// related to probability function caching
#if USE_PROB_FUNC_LOCAL_CACHE
	constexpr bool multithreaded = false;
	constexpr bool manythreads = false;
#else
	constexpr bool multithreaded = true;
	constexpr bool manythreads = true;
#endif

// data type specialications:
using num_t = int32_t;
using sum_t = int32_t;
using cnt_t = int16_t;
using prec_t = double;


#define USE_ONLY_3SAT_CLAUSES 0

#if USE_ONLY_3SAT_CLAUSES
	#include "sat/3sat-clause.hpp"
	using clause_t = static_clause_3sat<num_t, 4>;
#else
	#include "sat/clause.hpp"
	using clause_t = generic_clause<num_t>;
#endif

// #define DEBUG_WORKER_OUTPUT 1


using cnf_formula_t = cnf_formula<num_t, sum_t, cnt_t, clause_t>;

using poly_prob_func_t = prob_func_polynomial<num_t, prec_t>;
using exp_prob_func_t = prob_func_exponential<num_t, prec_t>;

#if USE_CACHED_PROB_FUNC
	using prob_func_t = prob_func_cached<num_t, prec_t, poly_prob_func_t, multithreaded, manythreads>;
#else
	using prob_func_t = poly_prob_func_t;
#endif

using random_generator_t = lin_cong_random_generator;
using serial_instance_t = serial_instance<num_t, sum_t, cnt_t, prec_t, clause_t, prob_func_t, random_generator_t, caching>;


static prob_func_t global_pfi = prob_func_t();
// thread_local prob_func_t global_pfi = prob_func_t();


class task {
	public:
		std::size_t max_flips;
		std::shared_ptr<cnf_formula_t> cnf_sptr;
		done_processing_info dpi;
		std::string name;

		prob_func_t *pfi_ptr;

		task(const problem_instance_metadata &pim, std::shared_ptr<cnf_formula_t> cnf, const uint32_t task_id, prob_func_t *pf = &global_pfi) :
			max_flips(default_max_flips), cnf_sptr(cnf), dpi(), name(pim.filename), pfi_ptr(pf)
		{
			dpi.solved = false;
			dpi.task_id = task_id;
			if (0 != pim.anzahl_flips) {
				max_flips = pim.anzahl_flips;
			}
		}
		
		// task(const task &o) : max_flips(o.max_flips), cnf_sptr(o.cnf_sptr), dpi(o.dpi), name(o.name), pfi(o.pfi) {}
		task(const task &other) = delete;

		void execute() {
			auto start = std::chrono::high_resolution_clock::now();

			try {
				#if DEBUG_WORKER_OUTPUT
					std::cout << msg_header << "c processing: " << name << std::endl;
				#endif

				assert(nullptr != cnf_sptr.get());

				#if ENALBE_CNF_MULTITHREAD_SHARING
				#else
					static_assert(false);
				#endif

				prob_func_t &pfi = *pfi_ptr;

				// requires ENALBE_CNF_MULTITHREAD_SHARING to be defined as 1 for correct multithreading
				// bformula.initialize();
				cnf_sptr->initialize();
				const cnf_formula_t &bformula = *(cnf_sptr.get());
				// bformula.debug_output_formula();

				dpi.num_vars = bformula.num_vars();
				dpi.num_clauses = bformula.num_clauses();

				random_generator_t rgen = random_generator_t();
				dpi.seed = rgen.get_seed();
				#if DEBUG_WORKER_OUTPUT
					std::cout << msg_header << "c seed = " << std::to_string(rgen.get_seed()) << std::endl;
				#endif

				auto belegung = serial_instance_t::configuration_type(bformula.num_vars());
				// for (num_t i = 0; i < bformula.num_vars(); i++) {belegung[i] = 0;}
				for (num_t i = 0; i < bformula.num_vars(); i++) {belegung[i] = rgen.rand() % 2;}

				serial_instance_t solver = serial_instance_t(bformula, pfi, belegung, rgen);


				auto initialized = std::chrono::high_resolution_clock::now();
				auto initialization_duration = std::chrono::duration_cast<std::chrono::milliseconds>(initialized - start);
				dpi.init_duration = std::chrono::duration_cast<std::chrono::microseconds>(initialized - start).count();
				#if DEBUG_WORKER_OUTPUT
					std::cout << msg_header << "c initialization duration: " << initialization_duration.count() << " ms" << std::endl;
				#endif


				auto start_solving = std::chrono::high_resolution_clock::now();

				while (!solver.found_solution()) {
					solver.do_flip();
					
					if (max_flips <= solver.get_num_flips()) {
						#if DEBUG_WORKER_OUTPUT
							std::cout << msg_header << "c FOUND NO SOLUTION AFTER " << solver.get_num_flips() << " FLIPS!" << std::endl;
						#endif
						break;
					}
					
					// if (!running) return;
					if (!running)
						break;
				}

				auto done_solving = std::chrono::high_resolution_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::microseconds>(done_solving - start_solving).count();
				dpi.solve_duration = std::chrono::duration_cast<std::chrono::microseconds>(done_solving - start_solving).count();

				auto num_flips = solver.get_num_flips();
				double fps = ((float) num_flips) / duration * 1000000.;
				
				dpi.num_flips_done = num_flips;
				dpi.flips_per_second = fps;

				#if DEBUG_WORKER_OUTPUT
					std::cout << msg_header << "c done after " << num_flips << " flips and " << round(duration/1000.) << " milliseconds" << std::endl;
					std::cout << msg_header << "c => " << round(fps) << " flips per seconds" << std::endl;
				#endif
				
				if (solver.found_solution()) {
					assert(solver.check_assignment());
					dpi.solved = true;
					#if DEBUG_WORKER_OUTPUT
						std::cout << msg_header << "s SATISFIABLE" << std::endl;
					#endif
				}
				
				// should prevent cnf_sptr getting optimized away while cnf_sptr.get() is in usage
				volatile auto a = (0 < cnf_sptr.use_count());
				assert(0 < a); ignore(a);
			} catch (const std::exception& ex) {
				print_exception(msg_header, ex);
				return;
			}

			auto done = std::chrono::high_resolution_clock::now();
			auto overall_duration = std::chrono::duration_cast<std::chrono::seconds>(done - start);
			dpi.overall_duration = std::chrono::duration_cast<std::chrono::microseconds>(done - start).count();
			#if DEBUG_WORKER_OUTPUT
				std::cout << msg_header << "c overall duration: " << overall_duration.count() << " seconds" << std::endl;
			#endif

			// should prevent cnf_sptr getting optimized away while cnf_sptr.get() is in usage
			dpi.instances_processed = (nullptr != cnf_sptr.get());
			assert(dpi.instances_processed == 1);
		}
};


std::mutex task_mutex;
std::deque<std::shared_ptr<task>> tasks;
std::mutex tasks_done_mutex;
std::deque<std::shared_ptr<task>> tasks_done;
uint32_t instances_to_get = 0;

std::condition_variable cv_main_mpi_thread;


void create_tasks(std::vector<job> jobs) {
	#if DEBUG_WORKER
		std::cout << msg_header << "creating " << jobs.size() << " jobs" << std::endl;
	#endif

	for (auto j : jobs) {
		assert(0 < j.num_instances_to_start);

		#if USE_CNF_MULTITHREAD_SHARING
			auto cnf_sptr = std::make_shared<cnf_formula_t>(j.metadata.filename, false);
			// std::cout << "POINTER: " << cnf_sptr.get() << std::endl;

			#if USE_FIX_CNF_GLOBAL_INPUT_READING
				cnf_sptr->initialize();
			#endif
			
			{ // task_mutex.lock();
				std::lock_guard<std::mutex> lock(task_mutex);
				for (uint32_t i = 0; i < j.num_instances_to_start; i++) {
					tasks.push_back(std::make_shared<task>(j.metadata, cnf_sptr, j.task_id));
				}

				instances_to_get -= j.num_instances_to_start;
			} // task_mutex.unlock();
		#else
			{ // task_mutex.lock();
				std::lock_guard<std::mutex> lock(task_mutex);
				for (uint32_t i = 0; i < j.num_instances_to_start; i++) {
					tasks.push_back(std::make_shared<task>(j.metadata, std::make_shared<cnf_formula_t>(j.metadata.filename, false), j.task_id));
				}

				instances_to_get -= j.num_instances_to_start;
			} // task_mutex.unlock();
		#endif

		#if DEBUG_WORKER
			std::cout << msg_header << "added " << j.num_instances_to_start << " tasks" << std::endl;
		#endif
	}
}


auto get_task() {
	while (running) {
		{ // task_mutex.lock();
			std::lock_guard<std::mutex> lock(task_mutex);
			if (tasks.empty()) {
				cv_main_mpi_thread.notify_all();
			} else {
				auto t = tasks.front();
				tasks.pop_front();
				return t;
			}
		} // task_mutex.unlock();

		std::this_thread::sleep_for(sleep_duration_between_get_task_polls);
	}
	
	return std::shared_ptr<task>();
}


void worker_func() {
	while (running) {
		try {
			std::shared_ptr<task> t = get_task();
			if (!running) return;

			#if DEBUG_WORKER
				std::cout << msg_header << "executing task" << std::endl;
			#endif

			assert(t);

			#if USE_PROB_FUNC_LOCAL_CACHE
				thread_local static prob_func_t tl_pfi = prob_func_t();
				t->pfi_ptr = &tl_pfi;
			#endif

			t->execute();
			// if (!running) return;

			#if DEBUG_WORKER
				std::cout << msg_header << "finished task" << std::endl;
			#endif
			
			{ // tasks_done_mutex.lock();
				std::lock_guard<std::mutex> lock(tasks_done_mutex);
				tasks_done.push_back(t);
				instances_to_get++;
			} // tasks_done_mutex.unlock();
		} catch (const std::exception& ex) {
			print_exception(msg_header, ex);
			return;
		}
	}
}


void return_task(std::shared_ptr<task> t) {
	if (t->dpi.solved) {
		#if DEBUG_WORKER
			std::cout << msg_header << "found solution!" << std::endl;
		#endif

		W2S::found_solution fs_cmd({t->dpi.seed, t->dpi.num_flips_done,
			t->dpi.solve_duration, t->dpi.num_vars, t->dpi.num_clauses, t->name});
		fs_cmd.send();
	}

	if (0 >= t->dpi.instances_processed) {
		throw std::runtime_error("worker was unable to solve task!");
	}

	#if DEBUG_COMMUNICATION
		std::cout << msg_header << "returning task " << (t->dpi.solved ? "" : "un")
			<< "solved after " << t->dpi.num_flips_done << " flips" << std::endl;
	#endif

	W2S::done_processing dp_cmd(t->dpi);
	dp_cmd.send();
}

#endif
