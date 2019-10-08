#include "server.h"

struct addrinfo get_addrinfo_hints() noexcept
{
	struct addrinfo hints;

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_addrlen = 0;
	hints.ai_addr = nullptr;
	hints.ai_canonname = nullptr;
	hints.ai_next = nullptr;

	return hints;
}

int get_binded_socket(struct addrinfo *address_info) noexcept
{
	int socket_fd = -1;
	bool bind_success = false;

	for (auto it = address_info; it; it = it->ai_next)
	{
		socket_fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (socket_fd == -1)
		{
			continue;
		}

		int yes = 1;
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
		{
			std::lock_guard<std::mutex> lock(cerr_mutex);
			LOG_CERROR("Program terminates due to setsockopt fail");
			exit(EXIT_FAILURE);
		}

		if (bind(socket_fd, it->ai_addr, it->ai_addrlen) == -1)
		{
			close(socket_fd);
			continue;
		}

		bind_success = true;
		break;
	}

	if (bind_success)
		return socket_fd;
	else
		return -1;
}

int get_listening_socket() noexcept
{
	struct addrinfo hints = get_addrinfo_hints();
	struct addrinfo *address_info;

	int gai_res = getaddrinfo(server_ip.data(), server_port.data(), &hints, &address_info);
	if (gai_res != 0)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		std::cerr << "Error of getaddrinfo: " << gai_strerror(gai_res) << "\n";
		exit(EXIT_FAILURE);
	}

	int socket_fd = get_binded_socket(address_info);

	freeaddrinfo(address_info);

	if (socket_fd == -1)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		std::cerr << "Failed to bind\n";
		exit(EXIT_FAILURE);
	}

	if (listen(socket_fd, SOMAXCONN) == -1)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		LOG_CERROR("Program terminates due to listen error");
		exit(EXIT_FAILURE);
	}

	std::clog << "Listening master socket fd is " << socket_fd << std::endl;

	return socket_fd;
}

void run_server_loop(int master_socket)
{
	size_t limit_of_file_descriptors = set_maximal_avaliable_limit_of_fd();
	std::clog << "Processing at most " << limit_of_file_descriptors << " fd at a time." << std::endl;

	initialize_thread_pool();

	while (true)
	{
		active_connection client(master_socket);

		if (!client)
		{
			continue;
		}

		worker_threads->enqueue_task(process_the_accepted_connection, std::move(client));
	}

	terminate_thread_pool();
}

void process_the_accepted_connection(active_connection client)
{
	constexpr size_t buffer_size = 8192;
	char buffer[buffer_size] = { 0 };

	ssize_t recieved = recv(client, buffer, buffer_size, MSG_NOSIGNAL);

	if (recieved > 0)
	{
		try
		{
			std::cout << "creating an http_request object..." << std::endl;

			http_request request(buffer);

			std::cout << "created http_request..." << std::endl;

			process_client_request(client, request);

			std::cout << "called process_request and got back from there" << std::endl;
		}
		catch (std::regex_error &re)
		{
			std::cout << "got a regex_error: " << re.what() << " that is " << typeid(re.code()).name() << std::endl;

			std::regex_constants::error_type error_type = re.code();

			switch (error_type)
			{
			case std::regex_constants::error_collate: std::cout << " 	the expression contains an invalid collating element name " << std::endl; break;
			case std::regex_constants::error_ctype : std::cout << " 	the expression contains an invalid character class name" << std::endl; break;

			case std::regex_constants::error_escape : std::cout << " 	the expression contains an invalid escaped character or a trailing escape" << std::endl; break;

			case std::regex_constants::error_backref : std::cout << " 	the expression contains an invalid back reference" << std::endl; break;

			case std::regex_constants::error_brack : std::cout << " 	the expression contains mismatched square brackets ('[' and ']')" << std::endl; break;

			case std::regex_constants::error_paren : std::cout << " 	the expression contains mismatched parentheses ('(' and ')')" << std::endl; break;

			case std::regex_constants::error_brace : std::cout << " 	the expression contains mismatched curly braces ('{' and '}')" << std::endl; break;

			case std::regex_constants::error_badbrace : std::cout << " 	the expression contains an invalid range in a {} expression" << std::endl; break;

			case std::regex_constants::error_range : std::cout << " 	the expression contains an invalid character range (e.g. [b-a])" << std::endl; break;

			case std::regex_constants::error_space : std::cout << " 	there was not enough memory to convert the expression into a finite state machine" << std::endl; break;

			case std::regex_constants::error_badrepeat : std::cout << " 	one of *?+{ was not preceded by a valid regular expression" << std::endl; break;

			case std::regex_constants::error_complexity : std::cout << " 	the complexity of an attempted match exceeded a predefined level" << std::endl; break;

			case std::regex_constants::error_stack : std::cout << " 	there was not enough memory to perform a match " << std::endl; break;

			default: std::cout << "UNKNOWN regex_error" << std::endl;
			}
		}
		catch (std::exception &e)
		{
			std::cout << "got an exception: " << e.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "caught an unknown exception" << std::endl;
		}
	}
	else if (recieved == -1)
	{
		std::cout << "\tgoing to report errors to err.log" << std::endl;
		std::lock_guard<std::mutex> lock(cerr_mutex);
		LOG_CERROR("Failed to recieve the request and process the client");
		std::cerr << "Client " << client << " remains unprocessed\n";
	}
}

void process_client_request(active_connection &client, http_request request)
{
	std::cout << "\t\tprocess_request starting to parse..." << std::endl;
	request.parse_request();
	std::cout << "\t\tparsed!!!!!!" << std::endl;

	std::string address = (server_directory + request.get_address()).data();

	std::cout << "Client requested file \"" << address << "\"" << std::endl;

	if (request)
	{
		open_file file(address.data());
		if (file)
		{
			if (request.status_required())
			{
				if (send_status_line(client, request.get_status()) == -1)
				{
					return;
				}
				if (send_headers(client, file) == -1)
				{
					return;
				}
			}

			send_client_a_file(client, file);
		}
		else
		{
			if (request.status_required())
			{
				send_status_line(client, 404);
			}
		}
	}
	else
	{
		if (request.status_required())
		{
			send_status_line(client, request.get_status());
		}
	}
	std::cout << "Request processed normaly" << std::endl;
}

const char *http_response_phrase(short status) noexcept
{
	static const std::map<short, const char *> responses
	{
		{ 200, "OK" },
		{ 400, "Bad Request" },
		{ 404, "Not Found" },
		{ 405, "Method Not Allowed" },
		{ 414, "URI Too Long" },
		{ 500, "Internal Server Error" },
		{ 505, "HTTP Version Not Supported" }
	};

	const char *result;
	try
	{
		result = responses.at(status);
	}
	catch (std::out_of_range &ex)
	{
		return "Unknown error of response status";
	}

	return result;
};

ssize_t send_status_line(active_connection &client, short status)
{
	constexpr char http_version[] = "HTTP/1.0";
	std::string status_line = http_version;
	status_line += ' ';
	status_line += std::to_string(status);
	status_line += ' ';
	status_line += http_response_phrase(status);
	status_line += "\r\n";

	return send(client, status_line.data(), status_line.size(), MSG_NOSIGNAL);
}

ssize_t send_headers(active_connection &client, open_file &file)
{
	std::string general_header;

	general_header += "Date: ";
	general_header += time_t_to_string(time_t_now());
	general_header += "\r\n";

	std::string response_header;

	response_header += "Location: ";
	response_header += file.location();
	response_header += "\r\n";
	response_header += "Server: Bolbot-CPPserver/10.0\r\n";

	std::string entity_header;

	const char allowed_methods[] = "GET";
	entity_header += "Allow: ";
	entity_header += allowed_methods;
	entity_header += "\r\n";

	entity_header += "Content-Length: ";
	entity_header += std::to_string(file.size());
	entity_header += "\r\n";
	entity_header += "Content-Type: ";
	entity_header += file.mime_type();
	entity_header += "\r\n";

	entity_header += "Expires: ";
	entity_header += time_t_to_string(time_t_now());
	entity_header += "\r\n";
	entity_header += "Last-Modified: ";
	entity_header += file.last_modified();
	entity_header += "\r\n";

	std::string total = general_header + response_header + entity_header + "\r\n";

	return send(client, total.data(), total.size(), MSG_NOSIGNAL);
}

void send_client_a_file(active_connection &client, open_file &file) noexcept
{
	constexpr size_t max_attempts = 3;

	for (size_t i = 0; i < max_attempts; ++i)
	{
		ssize_t file_sent = sendfile(client, file, nullptr, file.size());
		if (file_sent ==  -1 || file.size() == static_cast<size_t>(file_sent))
			break;
	}
}

time_t time_t_now() noexcept
{
	return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}
