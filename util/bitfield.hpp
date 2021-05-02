#ifndef BITFIELD_HPP
#define BITFIELD_HPP

// Implementierung eines Bitvektors um Belegungen kompakt repräsentieren zu können

#include <cstdint>
// #include <cstddef>
#include <cassert>
#include <type_traits>
#include <ostream>

template<typename length_t>
class bitfield {
	private:
		const length_t length; // number of bits

		// helper values to allow returning references to memory
		length_t last_mod_index;
		bool last_mod_value;

		uint8_t *array; // data


		// helper functions
		static inline length_t to_bytelength(const length_t l) {
			return (l+7) >> 3;
		}

		inline bool get(const length_t index) const {
			#if DEBUG_ASSERTIONS
			assert(index < length);
			#endif
			return 1 & (array[index >> 3] >> (index & 0b111));
		}

		inline void set(const length_t index, const bool value) {
			#if DEBUG_ASSERTIONS
			assert(index < length);
			#endif

			const uint8_t mask = 1 << (index & 0b111);
			if (value) {
				array[index >> 3] |= mask;
			} else {
				array[index >> 3] &= ~mask;
			}
		}

		inline void update() {
			if (last_mod_index < length) {
				// std::cout << "a["<<(int)last_mod_index<<"] = " << (int)last_mod_value << std::endl;
				set(last_mod_index, last_mod_value);
				last_mod_index = length;
			}
		}

	public:
		// constructors
		bitfield() : length(0), last_mod_index(length), last_mod_value(false), array(nullptr) {}
		
		bitfield(length_t n) :
			length(n), last_mod_index(length), last_mod_value(false),
			array(length > 0 ? new uint8_t[to_bytelength(length)] : nullptr) {}

		// copy constructor
		bitfield(const bitfield &other) :
			length(other.length),
			last_mod_index(other.last_mod_index),
			last_mod_value(other.last_mod_value),
			array(length > 0 ? new uint8_t[to_bytelength(length)] : nullptr)
		{
			for (length_t i = 0; i < to_bytelength(length); i++) {
				array[i] = other.array[i];
			}
		}
		
		// deconstructor
		~bitfield() {
			if (nullptr != array) {
				#if DEBUG_ASSERTIONS
					assert(0 < length);
				#endif
				delete[] array;
				array = nullptr;
			} else {
				#if DEBUG_ASSERTIONS
					assert(0 == length);
				#endif
			}
		}

		// copy & swap idiom
        friend void swap(bitfield &a, bitfield &b)
			noexcept(std::is_nothrow_swappable_v<length_t>
				&&   std::is_nothrow_swappable_v<bool>
				&&   std::is_nothrow_swappable_v<uint8_t*>)
		{
			#if DEBUG_ASSERTIONS
			assert(a.length == b.length);
			#endif
			// or remove const from length
            // std::swap(a.length, b.length);
            std::swap(a.last_mod_index, b.last_mod_index);
            std::swap(a.last_mod_value, b.last_mod_value);
            std::swap(a.array, b.array);
        }

        bitfield(bitfield &&other) : bitfield() {
            swap(*this, other);
        }

        bitfield& operator=(bitfield other) {
            swap(*this, other);
            return *this;
        }

		// access operators
		inline bool& operator[](const length_t index) {
			update();

			#if DEBUG_ASSERTIONS
			assert(index < length);
			#endif

			last_mod_index = index;
			last_mod_value = get(index);
			return last_mod_value;
		}
		
		inline bool operator[](const length_t index) const {
			if (index == last_mod_index) {
				return last_mod_value;
			}

			// TODO: comment might be problematic
			// update();
			return get(index);
		}
		
		// output
		std::ostream& print(std::ostream& os, const char *separator = " ") {
			update();
			
			const length_t bl = to_bytelength(length);

			auto sep = "";
			for (length_t i = 0; i < bl-1; i++) {
				os << sep;
				
				uint8_t d = array[i];
				for (uint8_t j = 0; j < 8; j++) {
					os << ((d & 1) ? "1" : "0");
					d >>= 1;
				}
				
				sep = separator;
			}
			
			if (0 < bl) {
				os << sep;
				for (length_t i = (bl-1)*8; i < length; i++) {
					os << (get(i) ? "1" : "0");
				}
			}
			
			return os;
        }
};

template<typename length_t>
std::ostream& operator<<(std::ostream& os, bitfield<length_t> bf) {
	return bf.print(os);
}

#endif
