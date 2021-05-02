
#include "config.hpp"

#include "util/util.hpp"
#include "sat/3sat-clause.hpp"
#include "sat/instance.hpp"
#include "sat/probability_functions/polynomial.hpp"
#include "sat/probability_functions/exponential.hpp"
#include "sat/probability_functions/cached.hpp"

#include <chrono>
#include <cmath>
#include <iostream>

// basic single threaded probsat solver

// g++ -Wall -Wextra -static -O3 -funroll-loops -fexpensive-optimizations -std=c++17 -Wall -o probsat probsat.cpp


// algorithm options:
constexpr bool caching = true;

constexpr bool multithreaded = true;
constexpr bool manythreads = true;


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

using cnf_formula_t = cnf_formula<num_t, sum_t, cnt_t, clause_t>;

using poly_prob_func_t = prob_func_polynomial<num_t, prec_t>;
using exp_prob_func_t = prob_func_exponential<num_t, prec_t>;

using prob_func_t = prob_func_cached<num_t, prec_t, poly_prob_func_t, multithreaded, manythreads>;
// alternative without caching:
// using prob_func_t = poly_prob_func_t;

// using random_generator_t = random_generator;
using random_generator_t = lin_cong_random_generator;
using serial_instance_t = serial_instance<num_t, sum_t, cnt_t, prec_t, clause_t, prob_func_t, random_generator_t, caching>;

// uint64_t max_flips = 500000000;
uint64_t max_flips = 20000000;

int main(int argc, char **argv)
{
	auto start = std::chrono::high_resolution_clock::now();
	
	try {
		std::string fname = "test.cnf";
		if (2 <= argc) {
			fname = argv[1];
		}

		auto seed = random_generator_t().get_seed();
		if (3 <= argc) {
			std::istringstream iss(argv[2]);
			iss >> seed;
			// seed = std::stoi(argv[2]);
		}

		std::cout << "c processing: " << fname << std::endl;

		cnf_formula_t bformula = cnf_formula_t(fname);
		std::cout << "c num vars = " << std::to_string(bformula.num_vars()) << std::endl;
		std::cout << "c clauses = " << std::to_string(bformula.num_clauses()) << std::endl;

		// prob_func_t pfi = poly_prob_func_t();
		prob_func_t pfi = prob_func_t(poly_prob_func_t());
		
		random_generator_t rgen = random_generator_t(seed);
		std::cout << "c seed = " << std::to_string(rgen.get_seed()) << std::endl;
		
		auto belegung = serial_instance_t::configuration_type(bformula.num_vars());
		// for (num_t i = 0; i < bformula.num_vars(); i++) {belegung[i] = 0;}
		// for (num_t i = 0; i < bformula.num_vars(); i++) {belegung[i] = rand() % 2;}
		for (num_t i = 0; i < bformula.num_vars(); i++) {belegung[i] = rgen.rand() % 2;}

		serial_instance_t solver = serial_instance_t(bformula, pfi, belegung, rgen);
		
		auto imported = std::chrono::high_resolution_clock::now();
		auto import_duration = std::chrono::duration_cast<std::chrono::milliseconds>(imported - start);
		
		std::cout << "c import duration: " << import_duration.count() << " ms" << std::endl;

		auto start_solving = std::chrono::high_resolution_clock::now();
		
		// int counter = 0;
		while (!solver.found_solution()) {
			solver.do_flip();
			
			if (max_flips <= solver.get_num_flips()) {
				std::cout << "c FOUND NO SOLUTION AFTER " << solver.get_num_flips() << " FLIPS!" << std::endl;
				break;
			}
			
			// int tmp = solver.get_num_flips() / 1000000;
			// if (tmp != counter) {
				// counter = tmp;
				// std::cout << "num_flips: " << solver.get_num_flips() << std::endl;
				// if (10000000 <= solver.get_num_flips())
					// break;
			// }
		}
		
		auto done_solving = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(done_solving - start_solving).count();
		
		auto num_flips = solver.get_num_flips();
		double fps = ((float) num_flips) / duration * 1000000.;
		double fpv = ((float) num_flips) / bformula.num_vars();
		double fpc = ((float) num_flips) / bformula.num_clauses();
		
		std::cout << "c done after " << num_flips << " flips and " << round(duration/1000.) << " milliseconds" << std::endl;
		std::cout << "c => " << round(fps) << " flips per seconds" << std::endl;
		std::cout << "c => " << fpv << " flips per variable" << std::endl;
		std::cout << "c => " << fpc << " flips per clause" << std::endl;
		
		if (solver.found_solution()) {
			assert(solver.check_assignment());
			std::cout << "s SATISFIABLE" << std::endl;
		}
	} catch (const std::exception& ex) {
		print_exception("main", ex);
	}

	auto done = std::chrono::high_resolution_clock::now();
	auto overall_duration = std::chrono::duration_cast<std::chrono::seconds>(done - start);
	std::cout << "c overall duration: " << overall_duration.count() << " seconds" << std::endl;
	
	return EXIT_SUCCESS;
}

