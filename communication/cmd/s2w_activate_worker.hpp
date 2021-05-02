#ifndef S2W_ACTIVATE_WORKER_HPP
#define S2W_ACTIVATE_WORKER_HPP

// Der Server aktiviert den Worker

class activate_worker {
	public:
		activate_worker() {}
		
	#if BUILD_SERVER
		static void send(int index, int id) {
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "sending ACTIVATE_WORKER command..." << std::endl;
			#endif

			MPI_Send(nullptr, 0, MPI_BYTE, id, ACTIVATE_WORKER, work_units[index]);
		}
	#endif

	#if BUILD_WORKER
		static void process(MPI_Status &status) {
			#if ADDITIONAL_CHECKS
				assert(ACTIVATE_WORKER == status.MPI_TAG);
			#else
				ignore(status);
			#endif

			// MPI_Recv(nullptr, 0, MPI_BYTE, MPI_ANY_SOURCE, ACTIVATE_WORKER, server_cl, &status);

			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "recieved ACTIVATE_WORKER command (" << status.MPI_TAG << ") from " << status.MPI_SOURCE << std::endl;
			#endif
		}
	#endif
};

#endif
