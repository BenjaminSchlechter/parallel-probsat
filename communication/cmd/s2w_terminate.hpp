#ifndef S2W_TERMINATE_HPP
#define S2W_TERMINATE_HPP

// Anweisung des Servers an den Worker zur Terminierung

class terminate {
	public:
		terminate() {}
		
	#if BUILD_SERVER
		static void send(int index, int id) {
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "sending TERMINATE command..." << std::endl;
			#endif

			MPI_Send(nullptr, 0, MPI_BYTE, id, TERMINATE, work_units[index]);
		}
	#endif

	#if BUILD_WORKER
		static void process(MPI_Status &status) {
			#if ADDITIONAL_CHECKS
				assert(TERMINATE == status.MPI_TAG);
			#endif

			MPI_Recv(nullptr, 0, MPI_BYTE, MPI_ANY_SOURCE, TERMINATE, server_cl, &status);

			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "recieved TERMINATE command (" << status.MPI_TAG << ") from " << status.MPI_SOURCE << std::endl;
			#endif
		}
	#endif
};

#endif
