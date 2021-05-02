#ifndef REQ_MAX_TRANSFER_SIZE_HPP
#define REQ_MAX_TRANSFER_SIZE_HPP

// Anfrage den Empfangspuffer zu vergrößern, generisch und von allen Prozessen verwendet

#include "../common.hpp"

#include <vector>
#include <cstdint>


template<int CTAG>
class req_max_transfer_size {
	public:
		std::size_t requested_size = default_max_cmd_length;

		req_max_transfer_size(std::size_t size) : requested_size(size) {
			assert(requested_size <= absolute_max_cmd_length);
		}

		void send(int id, MPI_Comm comm, std::vector<uint8_t> &buffer = command_buffer) {
			if (requested_size > default_max_cmd_length) {
				#if DEBUG_COMMUNICATION
					std::cout << msg_header << "sending REQ_MAX_CMD_LENGTH command (" << CTAG << ")" << std::endl;
				#endif
				
				assert(requested_size <= std::numeric_limits<int32_t>::max());
				four_byte_serializeable<int32_t> req_bufsize = {(int32_t)requested_size};

				static std::vector<uint8_t> fallback_buffer;
				// buffer.clear();
				auto data_len = bitsery::quickSerialization(output_adapter_t{fallback_buffer}, req_bufsize);
				assert(data_len <= default_max_cmd_length);

				MPI_Send(fallback_buffer.data(), data_len, MPI_BYTE, id, CTAG, comm);
				// small delay required?
				ignore(buffer);
			}
		}

		static std::size_t process(MPI_Status &status, std::vector<uint8_t> &buffer = command_buffer) {
			#if ADDITIONAL_CHECKS
				assert(CTAG == status.MPI_TAG);
			#endif

			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "recieved REQ_MAX_CMD_LENGTH command (" << status.MPI_TAG << ") from " << status.MPI_SOURCE << std::endl;
			#endif

			int count = get_bytes_recieved_safe(status, "REQ_MAX_CMD_LENGTH");

			four_byte_serializeable<int32_t> req_bufsize_;
			buffer.clear();
			safe_deserialization(input_adapter_t{buffer.begin(), (std::size_t) count}, req_bufsize_, "REQ_MAX_CMD_LENGTH");
			int32_t req_bufsize = req_bufsize_.num;

			if ((0 >= req_bufsize) || ((std::size_t) req_bufsize > absolute_max_cmd_length)) {
				throw std::runtime_error(std::string(msg_header) + "fatal error processing command REQ_MAX_CMD_LENGTH: requested buffer size "
					+ std::to_string(req_bufsize) + " is invalid, limits (0, " + std::to_string(absolute_max_cmd_length) + "]");
			}

			buffer.clear();
			buffer.reserve(req_bufsize);
			return req_bufsize;
		}
};

#endif
