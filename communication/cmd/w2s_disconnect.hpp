#ifndef W2S_DISCONNECT_HPP
#define W2S_DISCONNECT_HPP

// Der Worker wird die Verbindung zum Server beenden

class disconnect {
	public:
		disconnect() {}

	#if BUILD_WORKER
		static void send() {
			MPI_Send(nullptr, 0, MPI_BYTE, server_id, DISCONNECT, server_cl);
		}
	#endif

	#if BUILD_SERVER
		static void process(int index, MPI_Status &status) {
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "recieved DISCONNECT command (" << status.MPI_TAG << ") from " << status.MPI_SOURCE << std::endl;
			#endif

			#if ADDITIONAL_CHECKS
				assert(DISCONNECT == status.MPI_TAG);
			#endif
			
			ignore(index);
			ignore(status);
		}
	#endif
};

#endif

