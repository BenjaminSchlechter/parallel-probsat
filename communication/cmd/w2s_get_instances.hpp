#ifndef W2S_GET_INSTANCES_HPP
#define W2S_GET_INSTANCES_HPP

// Anfrage des Workers beim Server nach Probleminstanzen

struct get_instances_request {
	uint32_t num_instances_requested = 0;
};

template <typename S>
void serialize (S& s, get_instances_request &o) {
	s.value4b(o.num_instances_requested);
}


class get_instances {
	public:
		get_instances_request info;

		get_instances() {}
		get_instances(get_instances_request gi_info) : info(gi_info) {}

	#if BUILD_WORKER
		void send() {
			command_buffer.clear();
			auto data_len = bitsery::quickSerialization(output_adapter_t{command_buffer}, info);
			ensure_cmd_length(data_len);
			MPI_Send(command_buffer.data(), data_len, MPI_BYTE, server_id, W2S::GET_INSTANCES, server_cl);
		}
	#endif

	#if BUILD_SERVER
		void process(int index, MPI_Status &status) {
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "recieved GET_INSTANCES command (" << status.MPI_TAG << ") from " << status.MPI_SOURCE << std::endl;
			#endif

			#if ADDITIONAL_CHECKS
				assert(GET_INSTANCES == status.MPI_TAG);
			#endif

			int num_bytes = get_bytes_recieved_safe(status, "GET_INSTANCES", 0, worker_comm_buffers[index].capacity());
			safe_deserialization(input_adapter_t{worker_comm_buffers[index].begin(), (std::size_t) num_bytes}, info, "GET_INSTANCES");
			worker_comm_buffers[index].clear();
		}
	#endif
};

#endif

