#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

// gemeinsame Definitionen und Funktionen die zur Kommunikation verwendet werden

#include <string>
#include <iostream>
#include <vector>
#include <cstdint>

// binary serialization library (under MIT license)
// https://github.com/fraillt/bitsery
#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/string.h>

#include <mpi.h>

constexpr const std::size_t default_max_cmd_length = 1024;
constexpr const std::size_t absolute_max_cmd_length = 65536;

// nicht zwingend notwendig (buffergröße wird ggf. dynamisch angepasst...)
// static_assert(default_max_cmd_length > FILENAME_MAX);


using output_adapter_t = bitsery::OutputBufferAdapter<std::vector<uint8_t>>;
using input_adapter_t = bitsery::InputBufferAdapter<std::vector<uint8_t>>;

std::vector<uint8_t> command_buffer(default_max_cmd_length);


#if BUILD_MANAGER
	constexpr const char *msg_header = "manager: ";
#endif

#if BUILD_SERVER
	constexpr const char *msg_header = "server: ";
#endif

#if BUILD_WORKER
	constexpr const char *msg_header = "worker: ";
#endif


int get_count_recieved_safe(MPI_Status &status, MPI_Datatype dt, const std::string cmd,
	const int min = 1, const std::size_t max = std::numeric_limits<int>::max())
{
	static_assert(std::numeric_limits<std::size_t>::max() >= std::numeric_limits<int>::max());
	
	int count = 0;
	MPI_Get_count(&status, dt, &count);

	if ((min > count) || ((std::size_t) count > max)) {
		throw std::runtime_error(std::string(msg_header) + "command " + cmd + " recieved invalid amount of data (count: "
			+ std::to_string(count) + ", valid range: [" + std::to_string(min) + ", " + std::to_string(max) + "])");
	}

	return count;
}

int get_bytes_recieved_safe(MPI_Status &status, const std::string cmd,
	const int min = 1, const std::size_t max = std::numeric_limits<int>::max())
{
	return get_count_recieved_safe(status, MPI_BYTE, cmd, min, max);
}


template<class ia_t, class obj_t>
void safe_deserialization(ia_t &&input_adapter, obj_t &object, std::string cmd) {
	auto state = bitsery::quickDeserialization(std::move(input_adapter), object);
	if (state.first != bitsery::ReaderError::NoError || !state.second) {
		throw std::runtime_error(std::string(msg_header) + cmd + " command: deserialization failed");
	}
}


template<typename t>
struct four_byte_serializeable {
	t num;
};

template <typename S, typename t>
void serialize (S& s, four_byte_serializeable<t> &o) {
	s.value4b(o.num);
}


void print_buffer(std::vector<uint8_t> &buf) {
	std::cout << "buffer (" << buf.size() << " bytes):";
	for (uint8_t c : buf) { std::cout << " " << ((int32_t) c); }
	std::cout << std::endl;
}


#endif
