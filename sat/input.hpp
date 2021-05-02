#ifndef INPUT_HPP
#define INPUT_HPP

// Programmcode zum Einlesen einer CNF Formel

#include "../config.hpp"

#include <iostream>
#include <vector>
#include <cassert>


// Macros used:
// VERIFY_INPUT - check for common problems related to the input data
// ALLOW_UNCLEAN_CLAUSES - wheter a literal may appear multiple times in a clause
// DEBUG_OUTPUT - for development

#ifndef ALLOW_UNCLEAN_CLAUSES
#define ALLOW_UNCLEAN_CLAUSES 1
#endif

template<typename num_t, typename cnt_t, typename clause_t>
num_t read(std::istream& is, std::vector<clause_t> &clauses,
	std::vector<std::pair<cnt_t, cnt_t>> &count_clauses_with_vars);


// *********************************************************************
// Implementation:
// *********************************************************************

#include <string>
#include <sstream>
#include <limits>

#include "../util/util.hpp"


template<typename num_t>
inline void parse_configuration(std::istringstream &line_iss, std::string &line, std::string &field, num_t &num_vars, num_t &num_clauses);

template<typename num_t, typename cnt_t, typename clause_t>
inline void parse_clause(std::istringstream &line_iss, std::string &line, std::string &field,
	std::vector<clause_t> &clauses, std::vector<std::pair<cnt_t, cnt_t>> &count_clauses_with_vars);


template<typename num_t, typename cnt_t, typename clause_t>
num_t read(std::istream& is, std::vector<clause_t> &clauses,
	std::vector<std::pair<cnt_t, cnt_t>> &count_clauses_with_vars)
{
	// arguments:
	// input stream
	// vector to hold clauses
	// vector to count how often variables are used (pos/neg)
	// return number of vars
	
	// std::cin.sync_with_stdio(false);
	// std::cin.tie(0);

	static_assert(std::numeric_limits<num_t>::is_signed);
	static_assert(std::numeric_limits<num_t>::max() <= std::numeric_limits<std::ptrdiff_t>::max());
	static_assert(std::numeric_limits<num_t>::min() >= std::numeric_limits<std::ptrdiff_t>::min());
	static_assert(std::numeric_limits<num_t>::max() <= std::numeric_limits<typename clause_t::num_type>::max());
	static_assert(std::numeric_limits<num_t>::min() >= std::numeric_limits<typename clause_t::num_type>::min());

	num_t num_vars = 0;
	num_t num_clauses = 0;

	std::string line;
	line.reserve(128);
	std::string field;
	std::size_t linectr = 0;

	try {
		while (!is.eof()) {
			linectr++;
			
			std::getline(is, line);

			std::istringstream line_iss(line);
			
			switch(line[0]) {
				case 'c': // comment
					break;

				case 'p': // configuration
					parse_configuration<num_t>(line_iss, line, field, num_vars, num_clauses);
					clauses.reserve(num_clauses);
					count_clauses_with_vars.resize(num_vars);
					break;

				default: // parse clause
					if (line.empty()) {
						break;
					}
					
					parse_clause<num_t, cnt_t, clause_t>(line_iss, line, field, clauses, count_clauses_with_vars, num_vars);
					break;
			}
		}

		#if VERIFY_INPUT
			assert(std::numeric_limits<std::ptrdiff_t>::max() <= std::numeric_limits<std::size_t>::max());
			if ((std::size_t) num_clauses != clauses.size()) {
				#if VERIFY_INPUT && !ALLOW_UNCLEAN_CLAUSES
					#if DEBUG_OUTPUT
						std::cout << "some clauses which are always true have been removed" << std::endl;
					#endif
				#else
					throw std::runtime_error("invalid data: number of clauses doesn't match");
				#endif
			}
		#endif
	} catch (...) {
		std::throw_with_nested(std::runtime_error("fatal error while reading input, currently parsing in line " + std::to_string(linectr) + ":\n" + field + ""));
	}

	return num_vars;
}

template<typename num_t>
inline void verify_parameter(std::ptrdiff_t tmp) {
	#if VERIFY_INPUT
		if (1 > tmp) {
			throw std::runtime_error("less than one variable/clause doesn't make sense");
		}
	#endif

	if (tmp >= std::numeric_limits<num_t>::max()) {
		throw std::runtime_error("too many variables/clauses (recompile with different num_t)");
	}
}

template<typename num_t>
inline void parse_configuration(std::istringstream &line_iss, std::string &line, std::string &field, num_t &num_vars, num_t &num_clauses) {
	if (not starts_with(line, "p cnf ")) {
		throw std::runtime_error("invalid format: expected ' cnf num_vars num_clauses' following 'p'");
	}

	line_iss >> field;
	line_iss >> field;

	line_iss >> field;
	std::ptrdiff_t tmp = std::stoll(field);
	num_vars = tmp;
	verify_parameter<num_t>(num_vars);

	line_iss >> field;
	tmp = std::stoll(field);
	num_clauses = tmp;
	verify_parameter<num_t>(num_clauses);

	if (not line_iss.eof()) {
		throw std::runtime_error("invalid format: too many fields in p line");
	}
}

template<typename num_t, typename cnt_t, typename clause_t>
inline void parse_clause(std::istringstream &line_iss, std::string &line, std::string &field, std::vector<clause_t> &clauses,
	std::vector<std::pair<cnt_t, cnt_t>> &count_clauses_with_vars, num_t &num_vars)
{
	if (not ends_with(line, " 0")) {
		throw std::runtime_error("invalid format: each clause must be terminated with a 0");
	}

	line_iss.str(line.substr(0, line.length()-2));

	std::ptrdiff_t nvars = 0;
	while (not line_iss.eof()) {
		std::getline(line_iss, field, ' ');
		std::ptrdiff_t tmp = std::stoll(field);

		#if VERIFY_INPUT
			if ((0 == tmp) || (llabs(tmp) > num_vars)) {
				throw std::out_of_range("found nonexistent variable in clause");
			}
		#else
			ignore(num_vars);
		#endif
		
		std::size_t index = llabs(tmp)-1;
		#if VERIFY_INPUT
			assert(index < count_clauses_with_vars.size());
		#endif
		std::pair<cnt_t, cnt_t> &cnt = count_clauses_with_vars[index];

		if (0 > tmp) {
			#if VERIFY_INPUT
				if (cnt.first == std::numeric_limits<cnt_t>::max()) {
					throw std::out_of_range("read: cnt_t is too small!");
				}
			#endif
			cnt.first++;
		} else {
			#if VERIFY_INPUT
				if (cnt.second == std::numeric_limits<cnt_t>::max()) {
					throw std::out_of_range("read: cnt_t is too small!");
				}
			#endif
			cnt.second++;
		}

		nvars++;
	}

	#if VERIFY_INPUT
		if (1 > nvars) {
			throw std::runtime_error("each clause must have at least one variable");
		}
	#endif

	line_iss.seekg(0);
	using it_t = std::istream_iterator<std::ptrdiff_t>;

	auto new_clause = clause_t(it_t(line_iss), it_t(), nvars);

	#if VERIFY_INPUT && !ALLOW_UNCLEAN_CLAUSES
		bool is_unclean = false;
		bool is_always_true = false;
		
		for (std::size_t i = 0; i < new_clause.num_vars(); i++) {
			num_t lit_i = new_clause[i];
			num_t var_index_i = abs(lit_i)-1;

			for (std::size_t j = 0; j < i; j++) {
				num_t lit_j = new_clause[j];
				num_t var_index_j = abs(lit_j)-1;
				
				if (var_index_i == var_index_j) {
					if (lit_i == lit_j) {
						is_unclean = true;
					} else {
						is_always_true = true;
					}
				}
			}
		}

		if (false == is_always_true) {
			if (is_unclean) {
				#if DEBUG_OUTPUT
					std::cout << "unclean clause " << clauses.size() << ": " << new_clause << std::endl;
				#endif
				throw std::runtime_error("cant process unclean clause!");
			}

			clauses.push_back(new_clause);
		}
	#else
		clauses.push_back(new_clause);
	#endif
}


#endif
