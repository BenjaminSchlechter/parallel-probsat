#ifndef CLAUSE_HPP
#define CLAUSE_HPP

// Datenstruktur zur Repräsentation von Klauseln

#include <istream>
#include <string>
#include <vector>
#include <cassert>

#include "../util/util.hpp"


template<typename num_t>
class generic_clause {
	private:
		std::vector<num_t> clause;

	public:
		using num_type = num_t;
		static constexpr bool is_clause_t = true;

		// constructor
		generic_clause() : clause() {}

		template<typename Iterator>
		generic_clause(Iterator it1, Iterator it2, std::size_t num_vars) : clause({}) {
			clause.reserve(num_vars);
			std::copy(it1, it2, back_inserter(clause));

			#if VERIFY_INPUT || DEBUG_ASSERTIONS
				assert(clause.size() == num_vars);
				assert(0 < num_vars);
			#endif
		}
		
		// delete copy constructor
		generic_clause(const generic_clause &other) : clause(other.clause) {}

		// copy & swap idiom
        friend void swap(generic_clause &a, generic_clause &b)
			noexcept(std::is_nothrow_swappable_v<std::vector<num_t>>)
		{
			std::swap(a.clause, b.clause);
        }

        generic_clause(generic_clause &&other) : generic_clause() {
            swap(*this, other);
        }

        generic_clause& operator=(generic_clause other) {
            swap(*this, other);
            return *this;
        }

		// basic clause operations
		inline num_t& operator[](const uint8_t index) {
			return clause[index];
		}
		
		inline const num_t& operator[](const uint8_t index) const {
			return clause[index];
		}
		
		std::size_t num_vars() const {
			return clause.size();
		}
};

#if __cplusplus > 201703L
template<typename clause_t> requires (clause_t::is_clause_t)
#else
template<typename clause_t, bool a = clause_t::is_clause_t>
#endif
std::ostream& operator<<(std::ostream& os, clause_t c) {
	auto sep = "";
	for (std::size_t i = 0; i < c.num_vars(); i++) {
		std::cout << sep << c[i];
		sep = " ";
	}
	
	return os;
}

#endif
