#include "utils.h"
#include "server.h"

int main(int argc, char **argv)
{
	std::cout << "C++ multithreaded server. Simple version.\n";

	std::cout << "First of all, right here in main we'll try to create an http_request object out of " << argv[0] << "\n";
	http_request request_object(argv[0]);

	parse_program_options(argc, argv);

	daemonize();

	int master_socket = get_listening_socket();

	run_server_loop(master_socket);

	return 0;
}
