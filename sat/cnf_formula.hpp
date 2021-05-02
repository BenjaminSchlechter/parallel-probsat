#ifndef CNF_FORMULA_HPP
#define CNF_FORMULA_HPP

// Datenstruktur für die Formeln und Informationen die für statisches Caching bei ProbSAT verwendet werden

#include "../config.hpp"

#include <string>
#include <vector>

#if ENALBE_CNF_MULTITHREAD_SHARING
#include <mutex>
#endif


// Macros used:
// VERIFY_INPUT - check for common problems related to the input data
// USE_CONT_DATASTRUCT - wheter to use an alternative datastructure with continuous memory layout
// ADDITIONAL_CHECKS - additional sanity checks used while developing to ensure correctness


// important:
// USE_CONT_DATASTRUCT fails in combination with USE_CRIT_LITERAL_CACHING (instance.hpp) for
// ./test-probsat Random/cnf/Balint/unif-p12-k5-r21.117-v250-c5279-S9174904490903161459.cnf
// but not when run in debugger...


// examples for template parameters:
// using num_t = int32_t;
// using clause_t = static_clause_3sat<num_t, 4>;
// using clause_t = generic_clause<num_t>;

// num_t muss Anzahl der Klauseln und Anzahl der Literale fassen und signed sein
// sum_t muss Summe der Variablen über alle Klauseln fassen (num_t < sum_t::max() = Anzahl aller Zahlen in cnf Datei), unsigned
// cnt_t muss Anzahl Vorkommen je Variable fassen max(Anzahl negierte Literale, Anz. positive Lit.), unsigned


// TODO: add requirement? clause_t::num_t == num_t
#if __cplusplus > 201703L
template<typename num_t, typename sum_t, typename cnt_t, class clause_t> requires (clause_t::is_clause_t)
#else
template<typename num_t, typename sum_t, typename cnt_t, class clause_t, bool a = clause_t::is_clause_t>
#endif
class cnf_formula {
	private:
		num_t num_variables = 0;
		// cnt_t max_num_lit_occurrences = 0; // max. times a *literal* is used in clauses
		
		// Information pro Klausel
		std::vector<clause_t> clauses; // Liste der Klauseln (Literale pro Klausel mit Werte +- [1,...,num_vars])


		#if USE_CONT_DATASTRUCT
			std::vector<std::pair<sum_t, cnt_t>> occurrence_offset; // offset in clauses_with_vars and number of negative clauses
			std::vector<num_t> clauses_with_vars; // Liste mit Klauseln in denen die Variable als Literal vorkommt
			void cnf_formula_constructor_variant_cont_datastruct(std::vector<std::pair<cnt_t, cnt_t>> &count_clauses_with_vars);
		#else
			// Information pro Variable
			std::vector<std::pair<std::vector<num_t>, std::vector<num_t>>> clauses_with_vars;
			// für jede Variable eine Liste mit Klauseln in denen die Variable als negatives & positives Literal vorkommt
		#endif

		num_t max_num_breaks_possible = 0;

		#if ENALBE_CNF_MULTITHREAD_SHARING
			std::mutex init_mutex;
		#endif
		
		std::string file;

	public:
		num_t num_vars() const;
		num_t num_clauses() const;
		num_t get_max_num_breaks_possible() const;
		clause_t get_clause(num_t clause_index) const;

		auto get_iterators_for_clauses_with_vars(num_t var_index) const;
		auto get_iterators_for_clauses_with_vars(num_t var_index);

		void initialize();
		cnf_formula(std::string filename, const bool load = true);

		void debug_output_formula() const;
};


// *********************************************************************
// Implementation:
// *********************************************************************

#include <tuple>
#include <type_traits>

#include "../util/util.hpp"
#include "input.hpp"

#include <fstream>
#include <cstdint>


#if __cplusplus > 201703L
	#define CNF_FORMULA_TEMPLATE_ template<typename num_t, typename sum_t, typename cnt_t, class clause_t>
	#define CNF_FORMULA_CLASS_ cnf_formula<num_t, sum_t, cnt_t, clause_t>
#else
	#define CNF_FORMULA_TEMPLATE_ template<typename num_t, typename sum_t, typename cnt_t, class clause_t, bool a>
	#define CNF_FORMULA_CLASS_ cnf_formula<num_t, sum_t, cnt_t, clause_t, a>
#endif

// getters:

CNF_FORMULA_TEMPLATE_
num_t CNF_FORMULA_CLASS_::num_vars() const {
	return num_variables;
}

CNF_FORMULA_TEMPLATE_
num_t CNF_FORMULA_CLASS_::num_clauses() const {
	return clauses.size();
}

CNF_FORMULA_TEMPLATE_
num_t CNF_FORMULA_CLASS_::get_max_num_breaks_possible() const {
	return max_num_breaks_possible;
}

CNF_FORMULA_TEMPLATE_
clause_t CNF_FORMULA_CLASS_::get_clause(num_t clause_index) const {
	return clauses[clause_index];
}


#if USE_CONT_DATASTRUCT

	CNF_FORMULA_TEMPLATE_
	auto CNF_FORMULA_CLASS_::get_iterators_for_clauses_with_vars(num_t var_index) const {
		std::pair<sum_t, cnt_t> oo = occurrence_offset[var_index];
		sum_t offset = oo.first;
		cnt_t neg_count = oo.second;

		// std::cout << "get_iterators_for_clauses_with_vars: offset: " << offset << ", neg_count: "
			// << neg_count << ", pos_count: " << (occurrence_offset[var_index+1].first - offset - neg_count) << std::endl;

		auto neg_start = clauses_with_vars.begin() + offset;
		auto neg_end = neg_start + neg_count;
		auto pos_start = neg_end;
		auto pos_end = clauses_with_vars.begin() + occurrence_offset[var_index+1].first;

		return std::make_tuple(neg_start, neg_end, pos_start, pos_end);
	}

	CNF_FORMULA_TEMPLATE_
	auto CNF_FORMULA_CLASS_::get_iterators_for_clauses_with_vars(num_t var_index) {
		std::pair<sum_t, cnt_t> oo = occurrence_offset[var_index];
		sum_t offset = oo.first;
		cnt_t neg_count = oo.second;

		// std::cout << "get_iterators_for_clauses_with_vars: offset: " << offset << ", neg_count: "
			// << neg_count << ", pos_count: " << (occurrence_offset[var_index+1].first - offset - neg_count) << std::endl;

		auto neg_start = clauses_with_vars.begin() + offset;
		auto neg_end = neg_start + neg_count;
		auto pos_start = neg_end;
		auto pos_end = clauses_with_vars.begin() + occurrence_offset[var_index+1].first;

		return std::make_tuple(neg_start, neg_end, pos_start, pos_end);
	}

#else

	CNF_FORMULA_TEMPLATE_
	auto CNF_FORMULA_CLASS_::get_iterators_for_clauses_with_vars(num_t var_index) const {
		// first iterator pair of the tuple is for the list of negative literals, the second pair for the positive literals
		return std::make_tuple(
			clauses_with_vars[var_index].first.begin(),
			clauses_with_vars[var_index].first.end(),
			clauses_with_vars[var_index].second.begin(),
			clauses_with_vars[var_index].second.end());
	}

	CNF_FORMULA_TEMPLATE_
	auto CNF_FORMULA_CLASS_::get_iterators_for_clauses_with_vars(num_t var_index) {
		// first iterator pair of the tuple is for the list of negative literals, the second pair for the positive literals
		return std::make_tuple(
			clauses_with_vars[var_index].first.begin(),
			clauses_with_vars[var_index].first.end(),
			clauses_with_vars[var_index].second.begin(),
			clauses_with_vars[var_index].second.end());
	}

#endif

// CNF_FORMULA_TEMPLATE_
// cnt_t CNF_FORMULA_CLASS_::get_max_num_lit_occurrences() const {
	// return max_num_lit_occurrences;
// }


// constructor:
CNF_FORMULA_TEMPLATE_
CNF_FORMULA_CLASS_::cnf_formula(std::string filename, const bool load) {
	file = filename;

	if (load) {
		initialize();
	}
}


CNF_FORMULA_TEMPLATE_
void CNF_FORMULA_CLASS_::initialize() {
	#if ENALBE_CNF_MULTITHREAD_SHARING
		{
			std::lock_guard<std::mutex> lock(init_mutex);
			// init_mutex.lock();
			if (0 < clauses.size()) { // already initialized
				// init_mutex.unlock();
				return;
			}
		}
	#endif

	#if DEBUG_OUTPUT
		std::cout << "CNF_FORMULA_CLASS_::initialize()" << std::endl;
	#endif

	// Anzahl negierte/positive Vorkommen je Literal
	std::vector<std::pair<cnt_t, cnt_t>> count_clauses_with_vars;

	clauses.clear();

	std::ifstream filehandle;
	filehandle.open(file);
	num_variables = read<num_t, cnt_t, clause_t>(filehandle, clauses, count_clauses_with_vars);
	filehandle.close();

	#if USE_CONT_DATASTRUCT
		cnf_formula_constructor_variant_cont_datastruct(count_clauses_with_vars);
	#else
		clauses_with_vars.resize(num_vars());

		for (num_t clause_index = 0; clause_index < num_clauses(); clause_index++) {
			const clause_t clause = get_clause(clause_index);
			for (std::size_t i = 0; i < clause.num_vars(); i++) {
				num_t lit = clause[i];
				num_t var_index = abs(lit)-1;
				if (0 > lit) {
					clauses_with_vars[var_index].first.push_back(clause_index);
					max_num_breaks_possible = std::max(max_num_breaks_possible, (num_t) clauses_with_vars[var_index].first.size());
				} else {
					clauses_with_vars[var_index].second.push_back(clause_index);
					max_num_breaks_possible = std::max(max_num_breaks_possible, (num_t) clauses_with_vars[var_index].second.size());
				}
			}
		}
	#endif

	#if ENALBE_CNF_MULTITHREAD_SHARING
		init_mutex.unlock();
	#endif
}


#if USE_CONT_DATASTRUCT
	CNF_FORMULA_TEMPLATE_
	void CNF_FORMULA_CLASS_::cnf_formula_constructor_variant_cont_datastruct(std::vector<std::pair<cnt_t, cnt_t>> &count_clauses_with_vars) {
		#if ADDITIONAL_CHECKS
			std::vector<std::pair<std::vector<num_t>, std::vector<num_t>>> add_check_clauses_with_vars;
			add_check_clauses_with_vars.resize(num_vars());

			for (num_t clause_index = 0; clause_index < num_clauses(); clause_index++) {
				const clause_t clause = get_clause(clause_index);
				for (std::size_t i = 0; i < clause.num_vars(); i++) {
					num_t lit = clause[i];
					num_t var_index = abs(lit)-1;
					if (0 > lit) {
						add_check_clauses_with_vars[var_index].first.push_back(clause_index);
						max_num_breaks_possible = std::max(max_num_breaks_possible, (num_t) add_check_clauses_with_vars[var_index].first.size());
					} else {
						add_check_clauses_with_vars[var_index].second.push_back(clause_index);
						max_num_breaks_possible = std::max(max_num_breaks_possible, (num_t) add_check_clauses_with_vars[var_index].second.size());
					}
				}
			}
		#endif

		// max_num_lit_occurrences = 0;

		std::vector<std::pair<cnt_t, cnt_t>> currently_initialized_;
		currently_initialized_.reserve(num_vars());

		occurrence_offset.resize(num_vars()+1);
		// occurrence_offset[i] ist Summe von j=0 bis i-1 über count_clauses_with_vars[j]
		sum_t sum = 0;
		for (num_t i = 0; i < num_vars(); i++) {
			currently_initialized_.emplace_back(std::make_pair<cnt_t, cnt_t>(0, 0));
			std::pair<cnt_t, cnt_t> &num_clauses_with_var_i = count_clauses_with_vars[i];
			occurrence_offset[i].first = sum;
			occurrence_offset[i].second = num_clauses_with_var_i.first;
			std::size_t s = num_clauses_with_var_i.first + num_clauses_with_var_i.second;
			#if VERIFY_INPUT
				if ((std::size_t) sum > std::numeric_limits<sum_t>::max() - s) {
					throw std::out_of_range("cnf_formula: sum_t is too small!");
				}
			#endif
			sum += s;
			// max_num_lit_occurrences = std::max(max_num_lit_occurrences, std::max(num_clauses_with_var_i.first, num_clauses_with_var_i.second));
		}
		occurrence_offset[num_vars()].first = sum;
		occurrence_offset[num_vars()].second = 0;

		clauses_with_vars.resize(sum);

		for (num_t clause_index = 0; clause_index < num_clauses(); clause_index++) {
			const clause_t clause = get_clause(clause_index);
			for (std::size_t i = 0; i < clause.num_vars(); i++) {
				num_t lit = clause[i];
				num_t var_index = abs(lit)-1;
				auto it_tuple = get_iterators_for_clauses_with_vars(var_index);
				// std::cout << "num_vars(): " << num_vars() << std::endl;
				// std::cout << "var_index: " << var_index << std::endl;
				if (0 > lit) {
					assert((0 == ADDITIONAL_CHECKS) || ((std::get<1>(it_tuple) - std::get<0>(it_tuple)) > currently_initialized_[var_index].first));
					auto it = std::get<0>(it_tuple) + currently_initialized_[var_index].first;
					*it = clause_index;
					currently_initialized_[var_index].first++;
					max_num_breaks_possible = std::max(max_num_breaks_possible, (num_t) (std::get<1>(it_tuple) - std::get<0>(it_tuple)));
				} else {
					assert((0 == ADDITIONAL_CHECKS) || ((std::get<3>(it_tuple) - std::get<2>(it_tuple)) > currently_initialized_[var_index].second));
					auto it = std::get<2>(it_tuple) + currently_initialized_[var_index].second;
					*it = clause_index;
					currently_initialized_[var_index].second++;
					max_num_breaks_possible = std::max(max_num_breaks_possible, (num_t) (std::get<3>(it_tuple) - std::get<2>(it_tuple)));
				}
			}
		}

		#if ADDITIONAL_CHECKS
			for (num_t i = 0; i < num_vars(); i++) {
				auto cl = add_check_clauses_with_vars[i];
				auto it_tuple = get_iterators_for_clauses_with_vars(i);

				// std::cout << "variable " << i << " (" << cl.first.size() << ", " << cl.second.size() << ")" << std::endl;

				assert((std::get<1>(it_tuple) - std::get<0>(it_tuple)) >= 0);
				assert((std::get<3>(it_tuple) - std::get<2>(it_tuple)) >= 0);
				assert((std::size_t) (std::get<1>(it_tuple) - std::get<0>(it_tuple)) == cl.first.size());
				assert((std::size_t) (std::get<3>(it_tuple) - std::get<2>(it_tuple)) == cl.second.size());

				{
					// std::cout << "checking " << cl.first.size() << " clauses with negative literals: " << std::endl;
					auto it_start = std::get<0>(it_tuple);
					auto it_end = std::get<1>(it_tuple);
					// std::cout << "it_start " << it_start << std::endl;
					// std::cout << "it_end " << it_end << std::endl;
					// std::cout << "dist " << (it_end - it_start) << std::endl;
					for (auto it = cl.first.begin(); it != cl.first.end(); it++) {
						// std::cout << " " << *it;
						assert(*it_start == *it);
						it_start++;
					}
					// std::cout << std::endl;
					assert(it_start == it_end);
				}

				{
					// std::cout << "checking " << cl.second.size() << " clauses with positive literals: " << std::endl;
					auto it_start = std::get<2>(it_tuple);
					auto it_end = std::get<3>(it_tuple);
					for (auto it = cl.second.begin(); it != cl.second.end(); it++) {
						// std::cout << " " << *it;
						assert(*it_start == *it);
						it_start++;
					}
					// std::cout << std::endl;
					assert(it_start == it_end);
				}
			}
		#endif
	}
#endif

// debug function:
CNF_FORMULA_TEMPLATE_
void CNF_FORMULA_CLASS_::debug_output_formula() const {
	std::cout << "got " << clauses.size() << " clauses with " << num_vars() << " variables:\n" << std::endl;

	for (const auto c : clauses) {
		std::cout << c << std::endl;
	}
}

#endif
