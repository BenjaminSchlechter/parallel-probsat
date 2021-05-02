#ifndef S2M_FOUND_SOLUTION_HPP
#define S2M_FOUND_SOLUTION_HPP

// Benachrichtigung des Managers durch den Server dass eine Lösung gefunden wurde

#include "ws_typedefs.hpp"


class found_solution {
	public:
		solution_info info;

		found_solution() {}
		found_solution(solution_info sinfo) : info(sinfo) {}

	#if BUILD_SERVER
		void send() {
			command_buffer.clear();
			auto data_len = bitsery::quickSerialization(output_adapter_t{command_buffer}, info);
			ensure_cmd_length(data_len);
			MPI_Send(command_buffer.data(), data_len, MPI_BYTE, manager_id, S2M::FOUND_SOLUTION, manager_cl);
		}
	#endif

	#if BUILD_MANAGER
		void process(MPI_Status &status) {
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "recieved FOUND_SOLUTION command (" << status.MPI_TAG << ") from " << status.MPI_SOURCE << std::endl;
			#endif

			#if ADDITIONAL_CHECKS
				assert(FOUND_SOLUTION == status.MPI_TAG);
			#endif

			int num_bytes = get_bytes_recieved_safe(status, "FOUND_SOLUTION", 0, command_buffer.capacity());
			safe_deserialization(input_adapter_t{command_buffer.begin(), (std::size_t) num_bytes}, info, "FOUND_SOLUTION");
			command_buffer.clear();
		}
	#endif
};

#endif
