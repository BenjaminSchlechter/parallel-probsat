#ifndef PROB_FUNC_POLYNOMIAL
#define PROB_FUNC_POLYNOMIAL

#include <cmath>

template<typename num_t, typename prec_t>
class prob_func_polynomial {
	public:
		using prec_type = prec_t;
		static constexpr bool is_prob_func_impl_t = true;
		const prec_t eps;
		const prec_t cb;

		prob_func_polynomial(prec_t eps_ = 0.9, prec_t cb_ = 2.06) : eps(eps_), cb(cb_) {}
		prob_func_polynomial(const prob_func_polynomial&) = delete;

		prob_func_polynomial(const prob_func_polynomial &&o) noexcept
			: eps(std::move(o.eps)), cb(std::move(o.cb)) {}
		
		prec_t calc_prob_function(num_t num_breaks) const {
			return pow(eps + num_breaks, -cb);
		}
		
		void set_max_num_breaks_possible(num_t num_breaks) const { ignore(num_breaks); }
};

#endif

