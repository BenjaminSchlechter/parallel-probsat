#ifndef THREE_SAT_CLAUSE_HPP
#define THREE_SAT_CLAUSE_HPP

// Datenstrukturen zur Repräsentation von 3-SAT Klauseln (optimierte Darstellung)

#include "clause.hpp"

template<typename num_t, uint8_t align_border>
class static_clause_3sat {
	private:
		num_t clause[3];

		// add padding to align array of static_clause_3sat to align_border in bytes
		static constexpr std::size_t alignment = ((align_border - ((3*sizeof(clause)) % align_border)) % align_border);
		const uint8_t padding[alignment];

	public:
		using num_type = num_t;
		static constexpr bool is_clause_t = true;

		// constructor
		static_clause_3sat() : clause{}, padding{} {}

		template<typename Iterator>
		static_clause_3sat(Iterator it1, Iterator it2, std::size_t num_vars) : clause{}, padding{}
		{
			#if VERIFY_INPUT || DEBUG_ASSERTIONS
			if (3 != num_vars) {
				throw std::runtime_error("a 3-sat clause is expected, number of variables doesn't match");
			}
			#endif

			ignore(it2, num_vars);
			
			auto it = it1;
			for (std::size_t i = 0; i < 3; i++) {
				clause[i] = *it;
				it++;
			}
		}
		
		// delete copy constructor
		static_clause_3sat(const static_clause_3sat &other) :
			clause{other.clause[0], other.clause[1], other.clause[2]}, padding{} {}

		// copy & swap idiom
        friend void swap(static_clause_3sat &a, static_clause_3sat &b)
			noexcept(std::is_nothrow_swappable_v<num_t>)
		{
			static_assert(a.alignment == b.alignment);
			std::swap(a.clause[0], b.clause[0]);
			std::swap(a.clause[1], b.clause[1]);
			std::swap(a.clause[2], b.clause[2]);
        }

        static_clause_3sat(static_clause_3sat &&other) : static_clause_3sat() {
            swap(*this, other);
        }

        static_clause_3sat& operator=(static_clause_3sat other) {
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
		
		static uint8_t num_vars() {
			return 3;
		}
};

#endif
