#ifndef M2S_ADD_WORKERS_HPP
#define M2S_ADD_WORKERS_HPP

// Der Manager fügt dem Server Worker Prozesse hinzu

class add_workers {
	public:
		add_workers() {}
		
	#if BUILD_MANAGER
		static void send(const int num_workers) {
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "sending ADD_WORKERS command..." << std::endl;
			#endif

			MPI_Send(&num_workers, 1, MPI_INT, server_id, ADD_WORKERS, server_cl);
		}
	#endif

	#if BUILD_SERVER
		static int process(MPI_Status &status) {
			#if ADDITIONAL_CHECKS
				assert(ADD_WORKERS == status.MPI_TAG);
			#endif

			int num_workers = 0;
			MPI_Recv(&num_workers, 1, MPI_INT, MPI_ANY_SOURCE, ADD_WORKERS, manager_cl, &status);
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "recieved ADD_WORKERS command (" << status.MPI_TAG << ") from " << status.MPI_SOURCE << std::endl;
			#endif

			assert(1 == get_count_recieved_safe(status, MPI_INT, "ADD_WORKERS", 1, 1));

			return num_workers;
		}
	#endif
};

#endif
