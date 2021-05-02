#ifndef M2S_TERMINATE_HPP
#define M2S_TERMINATE_HPP

// Der Manager weist den Server an zu terminieren

class terminate {
	public:
		terminate() {}
		
	#if BUILD_MANAGER
		static void send() {
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "sending TERMINATE command..." << std::endl;
			#endif

			MPI_Send(nullptr, 0, MPI_BYTE, server_id, TERMINATE, server_cl);
		}
	#endif

	#if BUILD_SERVER
		static void process(MPI_Status &status) {
			#if ADDITIONAL_CHECKS
				assert(TERMINATE == status.MPI_TAG);
			#endif

			MPI_Recv(nullptr, 0, MPI_BYTE, MPI_ANY_SOURCE, TERMINATE, manager_cl, &status);

			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "recieved TERMINATE command (" << status.MPI_TAG << ") from " << status.MPI_SOURCE << std::endl;
			#endif
		}
	#endif
};

#endif
