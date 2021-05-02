#ifndef W2S_DONE_PROCESSING_HPP
#define W2S_DONE_PROCESSING_HPP

// Benachrichtigung des Servers durch den Worker dass die Bearbeitung einer Probleminstanz abgeschlossen ist

#include "ws_typedefs.hpp"


class done_processing {
	public:
		done_processing_info info;

		done_processing() {}
		done_processing(done_processing_info dp_info) : info(dp_info) {}
		
	#if BUILD_WORKER
		void send() {
			command_buffer.clear();
			auto data_len = bitsery::quickSerialization(output_adapter_t{command_buffer}, info);
			ensure_cmd_length(data_len);
			MPI_Send(command_buffer.data(), data_len, MPI_BYTE, server_id, W2S::DONE_PROCESSING, server_cl);
		}
	#endif

	#if BUILD_SERVER
		void process(int index, MPI_Status &status) {
			#if ADDITIONAL_CHECKS
				assert(DONE_PROCESSING == status.MPI_TAG);
			#endif

			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "recieved DONE_PROCESSING command (" << status.MPI_TAG << ") from " << status.MPI_SOURCE << std::endl;
			#endif

			int num_bytes = get_bytes_recieved_safe(status, "DONE_PROCESSING", 0, worker_comm_buffers[index].capacity());
			safe_deserialization(input_adapter_t{worker_comm_buffers[index].begin(), (std::size_t) num_bytes}, info, "DONE_PROCESSING");
			worker_comm_buffers[index].clear();
		}
	#endif
};

#endif
