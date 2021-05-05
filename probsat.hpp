#ifndef PROBSAT_HPP
#define PROBSAT_HPP

// utility header for probsat.cpp

#include "config.hpp"

#include "util/util.hpp"
#include "util/parse_params.hpp"
#include "sat/3sat-clause.hpp"
#include "sat/instance.hpp"
#include "sat/probability_functions/polynomial.hpp"
#include "sat/probability_functions/exponential.hpp"
#include "sat/probability_functions/cached.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>


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


std::size_t num_iterations = 1;
uint64_t max_flips = 0;
decltype(random_generator_t().get_seed()) seed;
prec_t eps = 0.9;
prec_t cb = 2.06;
std::size_t output_solution = 0;


void do_init_configuration(const cnf_formula_t &bformula, serial_instance_t::configuration_type &belegung, random_generator_t &rgen) {
	for (num_t i = 0; i < bformula.num_vars(); i++) {belegung[i] = rgen.rand() % 2;}
}

void parse_param_help(std::queue<std::string> &params) {
	std::cerr << "usage: ./probsat [parameters] boolean_formula.cnf" << std::endl;
	std::cout << "parameters:" << std::endl;
	std::cout << "\t--help\t\t-h\tprint this help message [flag]" << std::endl;
	std::cout << "\t--maxflips <m>\t\tmaximum number of flips per run [default: m = 0 = infinity]" << std::endl;
	std::cout << "\t--runs <n>\t\tnumber of runs to perform [default: n = 1]" << std::endl;
	std::cout << "\t--seed <seed>\t\tseed to use on first run [default: random]" << std::endl;
	std::cout << "\t--cb <cb>\t\tcb parameter for probability function [default: 2.06]" << std::endl;
	std::cout << "\t--eps <eps>\t\teps parameter for probability function [default: 0.9]" << std::endl;
	std::cout << "\t--printSolution\t-a\tif a solution is found, print its configuration [flag]" << std::endl;

	ignore(params);
	exit(EXIT_SUCCESS);
}

void parse_output_solution(std::queue<std::string> &params) {
	params.pop();
	output_solution = 20;
}

void parse_cb_eps(std::queue<std::string> &params) {
	std::string type = params.front(); params.pop();
	std::string pmsg = ("--cb" == type) ? "cb value" : "eps value";

	if (params.empty()) {
		std::string errmsg = pmsg + " is required";
		throw std::runtime_error(errmsg);
	}

	std::istringstream iss(params.front());
	if ("--cb" == type) {
		iss >> cb;
	} else {
		iss >> eps;
	}
	if (!iss) {
		std::string errmsg = "can't parse '"
			+ params.front() + "' as " + pmsg;
		throw std::runtime_error(errmsg);
	}

	params.pop();
}

void parse_maxflips_and_seed(std::queue<std::string> &params) {
	std::string type = params.front(); params.pop();
	std::string pmsg = ("--seed" == type) ? "seed" : "maximum number of flips";

	if (params.empty()) {
		std::string errmsg = pmsg + " is required";
		throw std::runtime_error(errmsg);
	}

	std::istringstream iss(params.front());
	if ("--seed" == type) {
		iss >> seed;
	} else {
		iss >> max_flips;
	}
	if (!iss) {
		std::string errmsg = "can't parse '"
			+ params.front() + "' as " + pmsg;
		throw std::runtime_error(errmsg);
	}

	params.pop();
}

void parse_num_iterations(std::queue<std::string> &params) {
	params.pop();

	if (params.empty()) {
		throw std::runtime_error("number of iterations is required");
	}

	std::istringstream iss(params.front());
	iss >> num_iterations;
	if (!iss) {
		std::string errmsg = "can't parse '"
			+ params.front() + "' as number of iterations";
		throw std::runtime_error(errmsg);
	}

	params.pop();
}

#endif
