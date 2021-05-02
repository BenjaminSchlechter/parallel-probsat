#ifndef S2W_SEND_INSTANCES_HPP
#define S2W_SEND_INSTANCES_HPP

// Der Server sendet Probleminstanzen an den Worker

#include "ws_typedefs.hpp"


class send_instances {
	public:
		std::vector<job> jobs;

		send_instances() {}

	#if BUILD_SERVER
		void send(int index, int id) {
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "sending SEND_INSTANCES command..." << std::endl;
			#endif

			worker_comm_buffers[index].clear();
			auto data_len = bitsery::quickSerialization(output_adapter_t{worker_comm_buffers[index]}, jobs);
			ensure_cmd_length(index, id, data_len); // id is status.MPI_SOURCE of request
			MPI_Send(worker_comm_buffers[index].data(), data_len, MPI_BYTE, id, SEND_INSTANCES, work_units[index]);
		}
	#endif

	#if BUILD_WORKER
		void process(MPI_Status &status) {
			#if ADDITIONAL_CHECKS
				assert(SEND_INSTANCES == status.MPI_TAG);
			#endif

			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "recieved SEND_INSTANCES command (" << status.MPI_TAG << ") from " << status.MPI_SOURCE << std::endl;
			#endif

			int num_bytes = get_bytes_recieved_safe(status, "SEND_INSTANCES", 0, command_buffer.capacity());
			safe_deserialization(input_adapter_t{command_buffer.begin(), (std::size_t) num_bytes}, jobs, "SEND_INSTANCES");
			command_buffer.clear();
		}
	#endif
};

#endif

