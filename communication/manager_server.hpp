#ifndef MANAGER_SERVER_HPP
#define MANAGER_SERVER_HPP

// Schnittstellendefinitionen der Kommunikation zwischen Manager und Server

#include "common.hpp"
#include "cmd/req_max_transfer_size.hpp"
#include "cmd/ws_typedefs.hpp"

#include <vector>


#if BUILD_MANAGER
	MPI_Comm server_cl;
	int server_id = 0;
	std::size_t current_server_max_cmd_length = default_max_cmd_length;
#endif


#if BUILD_SERVER
	MPI_Comm manager_cl;
	int manager_id = 0;
	std::size_t current_manager_max_cmd_length = default_max_cmd_length;
#endif


namespace M2S {
	enum CTAG_M2S : int {
		INVALID_CMD = 0, // to catch errors
		REQ_MAX_CMD_LENGTH = 1,
		ADD_WORKERS,
		ADD_FILE,
		TERMINATE
	};

	// include generic implementation:
	typedef req_max_transfer_size<REQ_MAX_CMD_LENGTH> req_max_transfer_size;

	#if BUILD_MANAGER
		void ensure_cmd_length(std::size_t length) {
			// minimize communication needs:
			if (length > current_server_max_cmd_length) {
				req_max_transfer_size req = req_max_transfer_size(length);
				req.send(server_id, server_cl);
				current_server_max_cmd_length = length;
			}
		}
	#endif

	// include implementations (needs to be in namespace!)
	#include "cmd/m2s_add_workers.hpp"
	#include "cmd/m2s_add_file.hpp"
	#include "cmd/m2s_terminate.hpp"
}


namespace S2M {
	enum CTAG_S2M : int {
		INVALID_CMD = 0, // to catch errors
		REQ_MAX_CMD_LENGTH = 1,
		FOUND_SOLUTION,
		SEND_STATISTICS,
		DISCONNECT
	};

	// include generic implementation:
	typedef req_max_transfer_size<REQ_MAX_CMD_LENGTH> req_max_transfer_size;

	#if BUILD_SERVER
		void ensure_cmd_length(std::size_t length) {
			// minimize communication needs:
			if (length > current_manager_max_cmd_length) {
				req_max_transfer_size req = req_max_transfer_size(length);
				req.send(manager_id, manager_cl);
				current_manager_max_cmd_length = length;
			}
		}
	#endif

	// include implementations (needs to be in namespace!)
	#include "cmd/s2m_found_solution.hpp"
	#include "cmd/s2m_send_statistics.hpp"
	#include "cmd/s2m_disconnect.hpp"
}

static_assert((int) M2S::REQ_MAX_CMD_LENGTH == (int) S2M::REQ_MAX_CMD_LENGTH);

#endif

