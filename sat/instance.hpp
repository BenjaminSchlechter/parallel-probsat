#ifndef INSTANCE_HPP
#define INSTANCE_HPP

// Klasse welche eine ProbSAT-Probleminstanz (Belegung, Programmcode für einen Flip) enthält

#include "../config.hpp"
#include "cnf_formula.hpp"

#include "random_generator.hpp"

#ifdef USE_BITFIELD
	#include "../util/bitfield.hpp"
#endif


// Macros used:
// USE_BITFIELD - wheter to use a bitfield to store the current cnf formula assignment
// USE_CRIT_LITERAL_CACHING - wheter to cache the critical literal in a clause
// ADDITIONAL_CHECKS - additional sanity checks used while developing to ensure correctness
// ALLOW_UNCLEAN_CLAUSES - wheter a literal may appear multiple times in a clause
// DEBUG_OUTPUT - for development


#if __cplusplus > 201703L
template<typename num_t, typename sum_t, typename cnt_t, typename prec_t,
	class clause_t, class prob_func_t, class random_generator_t, bool caching>
	requires (
		clause_t::is_clause_t
		&& prob_func_t::is_prob_func_impl_t
		&& std::is_same<prec_t, prob_func_t::prec_type>::value
		&& random_generator_t::is_random_generator_impl_t)
#else
template<typename num_t, typename sum_t, typename cnt_t, typename prec_t,
	class clause_t, class prob_func_t, class random_generator_t, bool caching, 
	bool requires = // requires keyword is c++20 code...
		( clause_t::is_clause_t
		&& prob_func_t::is_prob_func_impl_t
		&& std::is_same<prec_t, typename prob_func_t::prec_type>::value
		&& random_generator_t::is_random_generator_impl_t)>
#endif
class serial_instance {
	private:
		using cnf_formula_t = cnf_formula<num_t, sum_t, cnt_t, clause_t>;
		const cnf_formula_t &bformula; // cnf formula

		#ifdef USE_BITFIELD
			using belegung_t = bitfield<num_t>;
		#else
			using belegung_t = std::vector<bool>;
		#endif

		belegung_t belegung; // current variable configuration
		std::vector<num_t> unsat_clauses; // list of unsatisfied clause indices
		std::vector<num_t> unsat_clauses_location; // maps clause_index to location in unsat_clauses
		// if a clause is satisfied, location is negative count of satisfied literals in clause

		std::size_t num_flips_done = 0;

		// data used for caching:
		std::vector<cnt_t> num_breaks_by_flip; // number of breaked clauses by flip of variable

		#if USE_CRIT_LITERAL_CACHING
			std::vector<num_t> crit_literal_in_clause; // critical variable in each clause
		#endif

		prob_func_t &pfi;
		random_generator_t &rgenerator;
		

		void make_clause_unsat(const num_t clause_index);
		void make_clause_sat(const num_t clause_index);

		cnt_t get_num_breaks_by_flip(num_t literal, const bool caching_ = caching) const;

		prec_t calc_f(num_t num_breaks);
		num_t get_index_of_random_unsatisfied_clause() const;
		prec_t get_random_value(prec_t l) const;

		bool is_clause_satisfied(const clause_t &clause) const;
		num_t get_critical_literal(const clause_t &clause, const num_t dont_take_literal = 0) const;
		num_t get_first_satisfied_literal(const clause_t &clause, const num_t dont_take_literal = 0) const;
		
		auto num_sat_lit_in_clause(const clause_t &clause) const;
		auto get_num_sat_lit_in_clause(const num_t clause_index) const;

		void flip_literal(num_t literal);

		void initialize();

	public:

		serial_instance(const cnf_formula_t &bf_in_cnf, prob_func_t &prob_func_impl, belegung_t initial_config, random_generator_t &rgen = random_generator_t());
		serial_instance(const serial_instance&) = delete; // no copy constructor!

		void do_flip();
		bool found_solution() const;
		bool check_assignment() const;

		auto get_number_of_unsat_clauses();
		std::size_t get_num_flips();

		using configuration_type = belegung_t;
};


// *********************************************************************
// Implementation:
// *********************************************************************


#include <vector>
#include <tuple>
#include <type_traits>

#if __cplusplus > 201703L
	#define INSTANCE_TEMPLATE_ template<typename num_t, typename sum_t, \
		typename cnt_t, typename prec_t, class clause_t, class prob_func_t, class random_generator_t, bool caching>
	#define INSTANCE_CLASS_ serial_instance<num_t, sum_t, cnt_t, prec_t, clause_t, prob_func_t, random_generator_t, caching>
#else
	#define INSTANCE_TEMPLATE_ template<typename num_t, typename sum_t,  \
		typename cnt_t, typename prec_t, class clause_t, class prob_func_t, class random_generator_t, bool caching, bool requires>
	#define INSTANCE_CLASS_ serial_instance<num_t, sum_t, cnt_t, prec_t, clause_t, prob_func_t, random_generator_t, caching, requires>
#endif


INSTANCE_TEMPLATE_
void INSTANCE_CLASS_::make_clause_unsat(const num_t clause_index) {
	unsat_clauses_location[clause_index] = unsat_clauses.size();
	unsat_clauses.push_back(clause_index);

	#if DEBUG_OUTPUT
		std::cout << "make_clause_unsat: clause " << clause_index << " is now unsat" << std::endl;
		std::cout << "unsat_clauses.size(): " << unsat_clauses.size() << std::endl;
	#endif
}


INSTANCE_TEMPLATE_
void INSTANCE_CLASS_::make_clause_sat(const num_t clause_index) {
	num_t now_sat_cl = unsat_clauses_location[clause_index];

	// clause must be in unsat list!
	assert((0 == ADDITIONAL_CHECKS) || (0 <= now_sat_cl));
	assert((0 == ADDITIONAL_CHECKS) || (clause_index == unsat_clauses[now_sat_cl]));
	
	unsat_clauses_location[clause_index] = -1;

	#if DEBUG_OUTPUT
		std::cout << "make_clause_sat: clause " << clause_index << " is now sat" << std::endl;
	#endif

	// move the last unsat clause to this position
	num_t uclause = unsat_clauses.back();
	unsat_clauses.pop_back();
	if (unsat_clauses.empty()) {
		#if DEBUG_OUTPUT
			std::cout << "last unsat clause removed!" << std::endl;
		#endif
		assert((0 == ADDITIONAL_CHECKS) || (uclause == clause_index));
		return; // it was the last unsat clause! :)
	}
	
	if ((std::size_t) now_sat_cl == unsat_clauses.size()) {
		#if DEBUG_OUTPUT
			std::cout << "removed clause from the end of unsat_clauses list" << std::endl;
		#endif
		return; // clause was at the end...
	}

	#if DEBUG_OUTPUT
		std::cout << "moving clause " << uclause << " from " << unsat_clauses.size() << " to " << now_sat_cl << std::endl;
		std::cout << "unsat_clauses.size(): " << unsat_clauses.size() << std::endl;
	#endif

	assert((0 == ADDITIONAL_CHECKS) || ((std::size_t) now_sat_cl < unsat_clauses.size()));

	unsat_clauses_location[uclause] = now_sat_cl;
	unsat_clauses[now_sat_cl] = uclause;
}


INSTANCE_TEMPLATE_
cnt_t INSTANCE_CLASS_::get_num_breaks_by_flip(num_t literal, const bool caching_) const {

	num_t var_index = abs(literal)-1;
	if (caching_) {
		#if DEBUG_OUTPUT
			std::cout << "literal: " << literal
				<< ", ref num breaks: " << get_num_breaks_by_flip(literal, false)
				<< ", cache: " << num_breaks_by_flip[var_index] << std::endl;
		#endif

		#if ADDITIONAL_CHECKS
			assert(get_num_breaks_by_flip(literal, false) == num_breaks_by_flip[var_index]);
		#endif

		return num_breaks_by_flip[var_index];
	} else {
		cnt_t num_breaks = 0;

		auto it_tuple = bformula.get_iterators_for_clauses_with_vars(var_index);
		auto it_start = std::get<2>(it_tuple);
		auto it_end = std::get<3>(it_tuple);
		if (0 < literal) {
			// an unsat literal greater zero means the variable is currently false
			// so only negative literals can break
			it_start = std::get<0>(it_tuple);
			it_end = std::get<1>(it_tuple);
		} else {} // positive literals can break
		
		for (auto it = it_start; it != it_end; it++) {
			if (1 == get_num_sat_lit_in_clause(*it)) {
				num_breaks++;
				// TODO: adapt to ALLOW_UNCLEAN_CLAUSES...
			}
		}

		return num_breaks;
	}
}


INSTANCE_TEMPLATE_
prec_t INSTANCE_CLASS_::calc_f(num_t num_breaks) {
	return pfi.calc_prob_function(num_breaks);
}


INSTANCE_TEMPLATE_
num_t INSTANCE_CLASS_::get_index_of_random_unsatisfied_clause() const {
	auto ri = num_flips_done % unsat_clauses.size();
	return unsat_clauses[ri];
}


INSTANCE_TEMPLATE_
prec_t INSTANCE_CLASS_::get_random_value(prec_t l) const {
	// returns random number between [0, l)
	// return ((prec_t) rand()) / (RAND_MAX + 1u) * l;
	return ((prec_t) rgenerator.rand()) / (rgenerator.get_rand_max() + 1u) * l;
}


INSTANCE_TEMPLATE_
bool INSTANCE_CLASS_::is_clause_satisfied(const clause_t &clause) const {
	return (0 != get_first_satisfied_literal(clause));
}


INSTANCE_TEMPLATE_
inline num_t INSTANCE_CLASS_::get_critical_literal(const clause_t &clause, const num_t dont_take_literal) const {
	return get_first_satisfied_literal(clause, dont_take_literal);
}


INSTANCE_TEMPLATE_
inline num_t INSTANCE_CLASS_::get_first_satisfied_literal(const clause_t &clause, const num_t dont_take_literal) const {
	for (std::size_t i = 0; i < clause.num_vars(); i++) {
		num_t lit = clause[i];

		if (dont_take_literal != lit) {
			num_t var_index = abs(lit)-1;
			bool value = belegung[var_index];

			if ((0 > lit) ^ value)
				return lit; // at least this literal is satisfied
		}
	}
	
	return 0;
}


INSTANCE_TEMPLATE_
auto INSTANCE_CLASS_::num_sat_lit_in_clause(const clause_t &clause) const {
	auto n = 0;
	for (std::size_t i = 0; i < clause.num_vars(); i++) {
		num_t lit = clause[i];
		num_t var_index = abs(lit)-1;
		const bool value = belegung[var_index];

		if ((0 > lit) ^ value) { n++; }
	}
	
	return n;
}


INSTANCE_TEMPLATE_
auto INSTANCE_CLASS_::get_num_sat_lit_in_clause(const num_t clause_index) const {
	return num_sat_lit_in_clause(bformula.get_clause(clause_index));
}


INSTANCE_TEMPLATE_
void INSTANCE_CLASS_::flip_literal(num_t literal) {
	num_t rvar_index = abs(literal) - 1;

	#if DEBUG_OUTPUT
		std::cout << "flip literal " << literal << std::endl;
	#endif

	// Liste der unerfüllten Klauseln aktualisieren
	auto it_tuple_tmp = bformula.get_iterators_for_clauses_with_vars(rvar_index);
	auto it_tuple = it_tuple_tmp; // first iterator pair of it_tuple is list of negative literals, second pair is for positive literals
	if (0 < literal) {
		// an unsat literal greater zero means the variable was false and is now flipped to true
		// so only negative literals can break
	} else { // positive literals can break, swap iterator pairs
		it_tuple = std::make_tuple(
				std::get<2>(it_tuple_tmp),
				std::get<3>(it_tuple_tmp),
				std::get<0>(it_tuple_tmp),
				std::get<1>(it_tuple_tmp)
			);
	}

	// break
	for (auto it = std::get<0>(it_tuple); it != std::get<1>(it_tuple); it++) {
		num_t clause_index = *it;

		num_t ucl = unsat_clauses_location[clause_index];

		if (0 <= ucl) {
			// clause is already in unsat list, nothing to do
			assert((0 == ADDITIONAL_CHECKS) || (unsat_clauses[ucl] == clause_index));
		} else {
			// clause is not in unsat list - so it's currently satisfied
			if (-1 == ucl) {
				// but it was the last literal satisfied
				make_clause_unsat(clause_index);

				if (caching) {
					// the current variable may not break a clause any more if flipped
					num_breaks_by_flip[rvar_index]--;
				}

				#if USE_CRIT_LITERAL_CACHING // && ALLOW_UNCLEAN_CLAUSES
					crit_literal_in_clause[clause_index] = 0;
				#endif
			} else {
				// one satisfied literal less in clause
				unsat_clauses_location[clause_index]++;
			}

			if (caching) {
				if (-2 == ucl) {
					// as only one literal is left, it may break this clause now if flipped
					num_t crit_literal = get_critical_literal(bformula.get_clause(clause_index), -literal);
					// crit_literal shall not choose -literal (bit not yet flipped: literal = unsat => -literal = sat)
					
					#if ALLOW_UNCLEAN_CLAUSES
						if (0 == crit_literal) {
							// found no critical literal -> so it's an unclean clause with one "-literal" left
							num_breaks_by_flip[rvar_index]++;
							// temporarily increment num_breaks_by_flip - it will be decremented in case (-1 == ucl)
							#if USE_CRIT_LITERAL_CACHING
								crit_literal_in_clause[clause_index] = -literal;
							#endif
						} else {
							num_breaks_by_flip[abs(crit_literal) - 1]++;
							#if USE_CRIT_LITERAL_CACHING
								crit_literal_in_clause[clause_index] = crit_literal;
							#endif
						}
					#else
						assert((0 == ADDITIONAL_CHECKS) || (0 != crit_literal));
						num_breaks_by_flip[abs(crit_literal) - 1]++;
						#if USE_CRIT_LITERAL_CACHING
							crit_literal_in_clause[clause_index] = crit_literal;
						#endif
					#endif
				}
			}
		}
	}

	// make (must be done after break, so clauses with lit and -lit are correctly processed)
	for (auto it = std::get<2>(it_tuple); it != std::get<3>(it_tuple); it++) {
		num_t clause_index = *it;

		num_t ucl = unsat_clauses_location[clause_index];

		if (0 <= ucl) {
			// clause was in unsat list, make it satisfied
			make_clause_sat(clause_index);

			if (caching) {
				// the current variable may now break a clause if flipped again
				num_breaks_by_flip[rvar_index]++;
			}

			#if USE_CRIT_LITERAL_CACHING
				crit_literal_in_clause[clause_index] = literal;
			#endif
		} else {
			// clause is already satisfied
			
			// increment number of satisfied literals by one (counter is negative!)
			unsat_clauses_location[clause_index]--;

			if (caching) {
				if (-1 == ucl) {
					#if USE_CRIT_LITERAL_CACHING
						num_t crit_literal = crit_literal_in_clause[clause_index];
						num_t reference = get_critical_literal(bformula.get_clause(clause_index), -literal);
						#if DEBUG_OUTPUT
							std::cout << "USE_CRIT_LITERAL_CACHING: " << crit_literal
								<< ", reference: " << reference << std::endl;
						#endif
						assert((0 == ADDITIONAL_CHECKS) || (crit_literal == reference) || (crit_literal == literal));
					#else
						// this clause had a critical literal and now won't break any more by only one flip
						num_t crit_literal = get_critical_literal(bformula.get_clause(clause_index), -literal);
						// crit_literal shall not choose -literal (bit not yet flipped: literal = unsat => -literal = sat)
					#endif

					#if ALLOW_UNCLEAN_CLAUSES
						if (0 == crit_literal) {
							// if we can't find a critical literal, it's an unclean clause
							num_breaks_by_flip[rvar_index]--;
						} else {
							num_breaks_by_flip[abs(crit_literal) - 1]--;
						}

						#if USE_CRIT_LITERAL_CACHING
							crit_literal_in_clause[clause_index] = 0;
						#endif
					#else
						assert((0 == ADDITIONAL_CHECKS) || (0 != crit_literal));
						num_breaks_by_flip[abs(crit_literal) - 1]--;
					#endif
				}
			}
		}
	}

	// flip (must be done after make, so get_critical_literal in case of caching works correctly)
	bool value = belegung[rvar_index];
	belegung[rvar_index] = !value;

	#if DEBUG_OUTPUT
		std::cout << "flipped literal " << literal << " (var_index = " << rvar_index << ")" 
			<< " from " << (value ? "1": "0") << " to " << (belegung[rvar_index] ? "1" : "0") << std::endl;
	#endif
}


INSTANCE_TEMPLATE_
void INSTANCE_CLASS_::initialize() {
	#if DEBUG_OUTPUT
		std::cout << "create_initial_configuration():" << std::endl;
	#endif

	// Liste der unerfüllten Klauseln initialisieren
	auto num_clauses = bformula.num_clauses();

	// unsat_clauses may also grow dynamically so this is not really necessary but might be better for performance
	unsat_clauses.reserve(num_clauses);

	// required:
	unsat_clauses_location.reserve(num_clauses);
	if (caching) {
		#if DEBUG_OUTPUT
			std::cout << "use caching!" << std::endl;
		#endif
		num_breaks_by_flip.resize(bformula.num_vars());
		//f_cache.resize(bformula.get_max_num_breaks_possible()+1);
	}

	#if USE_CRIT_LITERAL_CACHING
		crit_literal_in_clause.resize(num_clauses);
	#endif

	pfi.set_max_num_breaks_possible(bformula.get_max_num_breaks_possible());

	for (num_t clause_index = 0; clause_index < num_clauses; clause_index++) {
		auto num_sat_lit = get_num_sat_lit_in_clause(clause_index);
		switch (num_sat_lit) {
			case 0:
				make_clause_unsat(clause_index);
				break;
			case 1:
				if (caching) {
					num_t crit_literal = get_critical_literal(bformula.get_clause(clause_index));
					num_breaks_by_flip[abs(crit_literal) - 1]++;

					// TODO: make caching compatible with ALLOW_UNCLEAN_CLAUSES...

					#if USE_CRIT_LITERAL_CACHING
						crit_literal_in_clause[clause_index] = crit_literal;
					#endif
				}
				[[fallthrough]];
			default:
				unsat_clauses_location[clause_index] = -num_sat_lit;
		}
	}
	
	#if DEBUG_OUTPUT
		std::cout << "cfg for formula with " << num_clauses << " clauses and " << bformula.num_vars() << " variables created" << std::endl;
		std::cout << "initial cfg has " << unsat_clauses.size() << " unsatisfied clauses\n" << std::endl;
	#endif
}



// public functions:

INSTANCE_TEMPLATE_
INSTANCE_CLASS_::serial_instance(const cnf_formula_t &bf_in_cnf, prob_func_t &prob_func_impl, belegung_t initial_config, random_generator_t &rgen)
	: bformula(bf_in_cnf), belegung(initial_config), pfi(prob_func_impl), rgenerator(rgen)
{
	static_assert(std::is_same<prec_t, typename prob_func_t::prec_type>::value);

	#if ADDITIONAL_CHECKS && DEBUG_OUTPUT
		std::cout << "ADDITIONAL_CHECKS = 1" << std::endl;
	#endif

	#if ALLOW_UNCLEAN_CLAUSES
		#if DEBUG_OUTPUT
			std::cout << "ALLOW_UNCLEAN_CLAUSES = 1" << std::endl;
		#endif
		// caching is currently not compatible with unclean clauses
		// static_assert(false == caching);
	#endif

	initialize();
}


INSTANCE_TEMPLATE_
void INSTANCE_CLASS_::do_flip() {
	// zufällig unerfüllte Klausel bestimmen
	num_t random_ci = get_index_of_random_unsatisfied_clause();
	const clause_t rclause = bformula.get_clause(random_ci);
	
	#if DEBUG_OUTPUT
		std::cout << "\n\nrandom unsat clause no " << random_ci << " in flip " << get_num_flips() << " is " << rclause << std::endl;

		for (std::size_t i = 0; i < rclause.num_vars(); i++) {
			num_t lit = rclause[i];
			num_t var_index = abs(lit)-1;
			const bool value = belegung[var_index];
				std::cout << "\t literal " << lit << " with var_index " << var_index
					<< " has value " << (value ? "1" : "0") << " and is " << (((0 > lit) ^ value) ? "sat" : "unsat") << std::endl;
		}

		std::cout << "num_sat_lit_in_clause(rclause): " << num_sat_lit_in_clause(rclause) << std::endl;
		std::cout << "is_clause_satisfied(rclause): " << (is_clause_satisfied(rclause) ? "1" : "0") << std::endl;
	#endif
	
	#if ADDITIONAL_CHECKS
		assert(0 == num_sat_lit_in_clause(rclause));
		assert(false == is_clause_satisfied(rclause));
	#endif
	
	// Funktion f(num_breaks) für jedes Literal bestimmen und aufsummieren
	static thread_local std::vector<prec_t> f;
	f.reserve(rclause.num_vars());
	prec_t sum_f = 0;
	// std::cout << "calculate sum_f:" <<  std::endl;
	for (std::size_t i = 0; i < rclause.num_vars(); i++) {
		num_t literal = rclause[i];
		cnt_t num_breaks = get_num_breaks_by_flip(literal);
		// std::cout << "\tnum_breaks: " << (int) num_breaks << std::endl;
		prec_t cf = calc_f(num_breaks);
		assert((0 == ADDITIONAL_CHECKS) || (0 < cf));
		// std::cout << "\tcf: " << cf << std::endl;
		sum_f += cf;
		f[i] = sum_f;
	}

	assert((0 == ADDITIONAL_CHECKS) || (0 < rclause.num_vars()));
	// zufälliges Literal auswählen
	prec_t r = get_random_value(sum_f); // must be in range [0, sum_f), important is != sum_f
	num_t rlit = 0;
	for (std::size_t i = 0; i < rclause.num_vars(); i++) {
		if (r < f[i]) {
			assert((0 == ADDITIONAL_CHECKS) || (0 == rlit));
			rlit = rclause[i];
			break;
		}
	}
	
	assert((0 == ADDITIONAL_CHECKS) || (0 != rlit));
	flip_literal(rlit);

	num_flips_done++;
}


INSTANCE_TEMPLATE_
bool INSTANCE_CLASS_::found_solution() const {
	return unsat_clauses.empty();
}


INSTANCE_TEMPLATE_
bool INSTANCE_CLASS_::check_assignment() const {
	for (num_t i = 0; i < bformula.num_clauses(); i++) {
		if (!is_clause_satisfied(bformula.get_clause(i))) {
			assert((0 == ADDITIONAL_CHECKS) || (0 == get_num_sat_lit_in_clause(i)));
			
			#if DEBUG_OUTPUT
				const clause_t uclause = bformula.get_clause(i);
				std::cout << "check_assignment: clause " << i << " is not satisfied (" << uclause << ")" << std::endl;
				for (std::size_t i = 0; i < uclause.num_vars(); i++) {
					num_t lit = uclause[i];
					num_t var_index = abs(lit)-1;
					const bool value = belegung[var_index];
					std::cout << "literal " << lit << " has value " << (value ? "1" : "0")
						<< " and is " << (((0 > lit) ^ value) ? "sat" : "unsat") << std::endl;
				}
			#endif
			
			return false;
		}
	}

	return true;
}


INSTANCE_TEMPLATE_
auto INSTANCE_CLASS_::get_number_of_unsat_clauses() {
	return unsat_clauses.size();
}


INSTANCE_TEMPLATE_
std::size_t INSTANCE_CLASS_::get_num_flips() {
	return num_flips_done;
}


#endif
