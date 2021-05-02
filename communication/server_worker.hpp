#ifndef SERVER_WORKER_HPP
#define SERVER_WORKER_HPP

// Schnittstellendefinitionen der Kommunikation zwischen Server und Worker

#include "common.hpp"
#include "cmd/req_max_transfer_size.hpp"

#include <vector>

#if BUILD_SERVER
	std::vector<MPI_Comm> work_units;
	std::vector<std::size_t> transfer_sizes;
	std::vector<std::vector<uint8_t>> worker_comm_buffers;
	std::size_t work_units_disconnected = 0;
#endif

#if BUILD_WORKER
	std::size_t current_server_max_cmd_length = default_max_cmd_length;
	MPI_Comm manager_cl;
	MPI_Comm server_cl;
	int server_id = 0;
#endif

namespace S2W {
	enum CTAG_S2W : int {
		INVALID_CMD = 0, // to catch errors
		REQ_MAX_CMD_LENGTH = 1,
		ACTIVATE_WORKER,
		SEND_INSTANCES,
		TERMINATE
	};

	// include generic implementation:
	typedef req_max_transfer_size<REQ_MAX_CMD_LENGTH> req_max_transfer_size;

	#if BUILD_SERVER
		void ensure_cmd_length(int index, int id, std::size_t length) {
			if (length > transfer_sizes[index]) {
				req_max_transfer_size req = req_max_transfer_size(length);
				req.send(id, work_units[index]);
				transfer_sizes[index] = length;
			}
		}
	#endif

	// include implementations (needs to be in namespace!)
	#include "cmd/s2w_activate_worker.hpp"
	#include "cmd/s2w_send_instances.hpp"
	#include "cmd/s2w_terminate.hpp"
}

namespace W2S {
	enum CTAG_W2S : int {
		INVALID_CMD = 0, // to catch errors
		REQ_MAX_CMD_LENGTH = 1,
		FOUND_SOLUTION,
		DONE_PROCESSING,
		GET_INSTANCES,
		DISCONNECT
	};

	typedef req_max_transfer_size<REQ_MAX_CMD_LENGTH> req_max_transfer_size;

	#if BUILD_WORKER
		void ensure_cmd_length(std::size_t length) {
			if (length > current_server_max_cmd_length) {
				req_max_transfer_size req = req_max_transfer_size(length);
				req.send(server_id, server_cl);
				current_server_max_cmd_length = length;
			}
		}
	#endif

	// include implementations (needs to be in namespace!)
	#include "cmd/w2s_found_solution.hpp"
	#include "cmd/w2s_done_processing.hpp"
	#include "cmd/w2s_get_instances.hpp"
	#include "cmd/w2s_disconnect.hpp"
}

static_assert((int) W2S::REQ_MAX_CMD_LENGTH == (int) S2W::REQ_MAX_CMD_LENGTH);

#endif






