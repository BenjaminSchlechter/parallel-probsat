#ifndef RANDOM_GENERATOR
#define RANDOM_GENERATOR

// Modularisierung um Zufallsgeneratoren einfach austauschen zu können
// enthält den Standard C rand() Generator und einen threadsicheren linearen Kongruenzgenerator

#include <cstdlib>
#include <random>

auto call_rand() {
	return rand();
}

class random_generator {
	public:
		using seed_t = decltype(time(NULL));

	private:
		// srand() & rand is not guaranteed to be thread-safe!
		const seed_t seed;
	
	public:
		static constexpr bool is_random_generator_impl_t = true;
		
		random_generator() : seed(time(NULL)) {
			srand(seed);
		}

		random_generator(const seed_t use_seed) : seed(use_seed) {
			srand(seed);
		}

		random_generator(const random_generator &other) = delete;
		random_generator(random_generator &&other) : seed(other.seed) {}

		auto get_seed() {
			return seed;
		}

		auto rand() {
			return call_rand();
		}

		auto get_rand_max() {
			return RAND_MAX;
		}
};


// produces linear congruential random numbers using:
// https://en.cppreference.com/w/cpp/numeric/random/linear_congruential_engine
// minstd_rand(C++11) 	std::linear_congruential_engine<std::uint_fast32_t, 48271, 0, 2147483647>
// Newer "Minimum standard", recommended by Park, Miller, and Stockmeyer in 1993
class lin_cong_random_generator {
	private:
		static std::random_device r;

	public:
		using seed_t = decltype(r());
	private:
		const seed_t seed;
		std::minstd_rand engine;

	public:
		static constexpr bool is_random_generator_impl_t = true;
		
		lin_cong_random_generator() : seed(r()), engine() {
			engine.seed(seed);
		}

		lin_cong_random_generator(const seed_t use_seed) : seed(use_seed), engine() {
			engine.seed(seed);
		}

		lin_cong_random_generator(const lin_cong_random_generator &other) = delete;
		lin_cong_random_generator(lin_cong_random_generator &&other) : seed(std::move(other.seed)), engine(std::move(other.engine)) {}

		auto get_seed() {
			return seed;
		}

		auto rand() {
			return engine() - engine.min();
		}

		auto get_rand_max() {
			return engine.max() - engine.min();
		}
};

std::random_device lin_cong_random_generator::r = std::random_device();

#endif
