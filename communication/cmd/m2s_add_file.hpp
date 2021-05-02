#ifndef M2S_ADD_FILE_HPP
#define M2S_ADD_FILE_HPP

// Der Manager übergibt eine Probleminstanz an den Server

#include "ws_typedefs.hpp"


class add_file {
	public:
		problem_instance_metadata data;

		add_file() : data() {}
		add_file(problem_instance_metadata pim) : data(pim) {}
		
	#if BUILD_MANAGER
		void send() {
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "sending ADD_FILE command..." << std::endl;
			#endif

			command_buffer.clear();
			auto data_len = bitsery::quickSerialization(output_adapter_t{command_buffer}, data);
			ensure_cmd_length(data_len);
			MPI_Send(command_buffer.data(), data_len, MPI_BYTE, server_id, ADD_FILE, server_cl);
		}
	#endif

	#if BUILD_SERVER
		void process(MPI_Status &status) {
			#if ADDITIONAL_CHECKS
				assert(ADD_FILE == status.MPI_TAG);
			#endif

			command_buffer.clear();
			MPI_Recv(command_buffer.data(), command_buffer.capacity(), MPI_BYTE, MPI_ANY_SOURCE, ADD_FILE, manager_cl, &status);

			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "recieved ADD_FILE command (" << status.MPI_TAG << ") from " << status.MPI_SOURCE << std::endl;
			#endif

			int num_bytes = get_bytes_recieved_safe(status, "ADD_FILE", 1, command_buffer.capacity());
			safe_deserialization(input_adapter_t{command_buffer.begin(), (std::size_t) num_bytes}, data, "ADD_FILE");
		}
	#endif
};

#endif
