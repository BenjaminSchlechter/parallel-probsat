#ifndef S2M_DISCONNECT_HPP
#define S2M_DISCONNECT_HPP

// Der Server informiert den Manager dass die Verbindung beendet wird

class disconnect {
	public:
		disconnect() {}

	#if BUILD_SERVER
		static void send() {
			MPI_Send(nullptr, 0, MPI_BYTE, manager_id, DISCONNECT, manager_cl);
		}
	#endif

	#if BUILD_MANAGER
		static void process(MPI_Status &status) {
			#if DEBUG_COMMUNICATION
				std::cout << msg_header << "recieved DISCONNECT command (" << status.MPI_TAG << ") from " << status.MPI_SOURCE << std::endl;
			#endif

			#if ADDITIONAL_CHECKS
				assert(DISCONNECT == status.MPI_TAG);
			#endif

			ignore(status);
		}
	#endif
};

#endif

