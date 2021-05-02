#ifndef S2M_SEND_STATISTICS_HPP
#define S2M_SEND_STATISTICS_HPP

// Übertragung der Ausführungsstatistik vom Server an den Manager

#include "ws_typedefs.hpp"


struct report_entry {
	problem_instance_metadata metadata;
	problem_instance_statistics statistics;
};

template <typename S>
void serialize (S& s, report_entry& o) {
	serialize(s, o.metadata);
	serialize(s, o.statistics);
}


class send_statistics {
	public:
		report_entry data;

		send_statistics() : data() {}
		send_statistics(report_entry re) : data(re) {}
	
	#if BUILD_SERVER	
		void send() {
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "sending SEND_STATISTICS command..." << std::endl;
			#endif

			command_buffer.clear();
			auto data_len = bitsery::quickSerialization(output_adapter_t{command_buffer}, data);
			ensure_cmd_length(data_len);
			MPI_Send(command_buffer.data(), data_len, MPI_BYTE, manager_id, SEND_STATISTICS, manager_cl);
		}
	#endif

	#if BUILD_MANAGER	
		void process(MPI_Status &status) {
			#if SEND_STATISTICS
				assert(ADD_FILE == status.MPI_TAG);
			#endif

			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "recieved SEND_STATISTICS command (" << status.MPI_TAG << ") from " << status.MPI_SOURCE << std::endl;
			#endif

			int num_bytes = get_bytes_recieved_safe(status, "SEND_STATISTICS", 1, command_buffer.capacity());
			safe_deserialization(input_adapter_t{command_buffer.begin(), (std::size_t) num_bytes}, data, "SEND_STATISTICS");
		}
	#endif
};

#endif
