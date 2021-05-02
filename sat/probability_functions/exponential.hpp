#ifndef PROB_FUNC_EXPONENTIAL
#define PROB_FUNC_EXPONENTIAL

template<typename num_t, typename prec_t>
class prob_func_exponential {
	public:
		using prec_type = prec_t;
		static constexpr bool is_prob_func_impl_t = true;
		const prec_t cb;

		prob_func_exponential(prec_t cb_ = 2.06) : cb(cb_) {}
		prob_func_exponential(const prob_func_exponential&) = delete;

		prob_func_exponential(const prob_func_exponential &&o) noexcept : cb(std::move(o.cb)) {}
		
		prec_t calc_prob_function(num_t num_breaks) const {
			return pow(cb, -num_breaks);
		}
		
		void set_max_num_breaks_possible(num_t num_breaks) const { ignore(num_breaks); }
};

#endif
