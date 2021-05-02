#ifndef PROB_FUNC_CACHED
#define PROB_FUNC_CACHED

#include <vector>
#include <mutex>

template<typename num_t, typename prec_t, typename prob_func_implementation_t, bool multithreaded = false, bool manythreads = false>
class prob_func_cached {
	std::vector<prec_t> cache;
	const prob_func_implementation_t pfi;
	std::mutex mtx;

	public:
		using prec_type = prec_t;
		static constexpr bool is_prob_func_impl_t = true;

		prob_func_cached(const prob_func_implementation_t &&prob_func_impl)
			: cache(), pfi(std::move(prob_func_impl)), mtx()
		{
			// false == (manythreads && (false == multithreaded))
			static_assert(!manythreads || multithreaded);
		}

		prob_func_cached() : prob_func_cached(prob_func_implementation_t()) {}

		prob_func_cached(const prob_func_cached&) = delete;

		prec_t calc_prob_function(num_t num_breaks) {
			prec_t f = pfi.calc_prob_function(num_breaks);

			assert((0 == ADDITIONAL_CHECKS) || (0 <= num_breaks));
			assert((0 == ADDITIONAL_CHECKS) || ((std::size_t) num_breaks < cache.size()));

			// read only access shouldn't require locking, but might be problematic?
			prec_t v = cache[num_breaks];
			if (0. < v) {
				assert((0 == ADDITIONAL_CHECKS) || (f == v));
				return v;
			} else {
				if (multithreaded) {
					if (manythreads) {
						// variant: dont wait until data can be written to cache,
						// might have overall better performance if many threads are used
						{
							std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
							if (lock.try_lock()) {
								cache[num_breaks] = f;
							}
						}
						// if (mtx.try_lock()) {
							// cache[num_breaks] = f;
							// mtx.unlock();
						// }
					} else {
						{ // mtx.lock();
							std::lock_guard<std::mutex> lock(mtx);
							cache[num_breaks] = f;
						} // mtx.unlock();
					}
				} else {
					cache[num_breaks] = f;
				}
			}

			return f;
		}

		void set_max_num_breaks_possible(num_t num_breaks) {
			// std::cout << "max_num_breaks_possible: " << num_breaks << std::endl;
			assert((0 == ADDITIONAL_CHECKS) || (0 <= num_breaks));
			if (cache.size() <= (std::size_t) num_breaks) {
				if (multithreaded) {
					{ // mtx.lock();
						std::lock_guard<std::mutex> lock(mtx);
						cache.resize(num_breaks + 1);
					} // mtx.unlock();
				} else {
					cache.resize(num_breaks + 1);
				}
			}
		}
};

#endif
