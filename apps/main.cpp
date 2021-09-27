#include "utils.h"		/* parse_program_options() and daemonize()*/
#include "server.h"		/* get_listening_socket() and run_server_loop() */

int main(int argc, char **argv)
{
	std::cout << "C++ multithreaded server. Simple version.\n";

	parse_program_options(argc, argv);

//	daemonize();

	int master_socket = get_listening_socket();

	run_server_loop(master_socket);

	return 0;
}
