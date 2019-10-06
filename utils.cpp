#include "utils.h"

std::mutex cerr_mutex;

std::string server_ip;
std::string server_port;
std::string server_directory;

void parse_program_options(int argc, char **argv) noexcept
{
	/*
	static constexpr size_t needed_arguments = 7;

	if (argc != needed_arguments)
	{
		std::cout << "Usage: " << argv[0] << " -h <ip> -p <port> -d <directory>\n";
		exit(EXIT_SUCCESS);
	}
	else
	{
		const char avaliable_options[] = "h:p:d:";
		int current;
		while ((current = getopt(argc, argv, avaliable_options)) != -1)
		{
			if (current == 'h')
			{
				server_ip = optarg;
			}
			else if (current == 'p')
			{
				server_port = optarg;
			}
			else
			{
				server_directory = optarg;
				if (server_directory.back() != '/')
				{
					server_directory.push_back('/');
				}
			}
		}
	}
	*/

	try
	{
		boost::program_options::options_description options("Call with following obligatory arguments");
		options.add_options()
			("host,h", boost::program_options::value<std::string>(&server_ip), "IP of server (i. e. 127.0.0.1)")
			("port,p", boost::program_options::value<std::string>(&server_port), "Port (use in range 1024..65535)")
			("directory,d", boost::program_options::value<std::string>(&server_directory), "Directory");

		boost::program_options::variables_map map;
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, options), map);
		boost::program_options::notify(map);

		if (!map.count("host") || !map.count("port") || !map.count("directory"))
		{
			std::lock_guard<std::mutex> lock(cerr_mutex);
			std::cerr << options << "\n";
			exit(EXIT_SUCCESS);
		}
		if (server_ip.empty() || server_port.empty() || server_directory.empty())
			throw std::runtime_error("Failed to parce given comand line arguemnts");
	}
	catch (std::exception &e)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		std::cerr << "Command-line arguments error: " << e.what() << ". Terminating.\n";
		exit(EXIT_FAILURE);	
	}
	catch (...)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		std::cerr << "Unknown error in command-line parser. Terminating.\n";
		exit(EXIT_FAILURE);
	}
}

void daemonize() noexcept
{
	pid_t pid = fork();

	if (pid == -1)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		std::cerr << "Error creating child process in damonize.\n";
		exit(EXIT_FAILURE);
	}
	else if (pid != 0)
	{
		exit(EXIT_SUCCESS);
	}

	umask(0);

	set_signals();

	try
	{
		log_redirector::instance();
	}
	catch (std::exception &e)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		std::cerr << "Failed to redirect output to log files: " << e.what() << "\n";
	}
	catch (...)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		std::cerr << "Unknown error while redirecting output to log files.\n";
	}

	pid_t sid = setsid();

	if (sid == -1)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		LOG_CERROR("program terminates due to the setsid failure");
		exit(EXIT_FAILURE);
	}

	int chdir_res = chdir("/");
	if (chdir_res == -1)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		LOG_CERROR("program terminates due to the chdir failure");
		exit(EXIT_FAILURE);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	if (std::atexit(atexit_terminator))
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		std::cerr << "Failed to set function for terminating threads at exit: atexit_terminator()\n";
	}
	std::set_terminate(terminate_handler);

	std::clog << "Daemoized successfully. " << time_t_to_string(current_time_t())
		<< "\nMaster process id " << getpid()
		<< "\nServer IP " << server_ip << "\nServer port " << server_port
		<< "\nServer directory " << server_directory << std::endl;
}

void log_errno(const char *function, const char *file, size_t line, const char *message) noexcept
{
	// cerr_mutex is locked before call to this function
	constexpr size_t buffer_size = 1024;
	char buffer[buffer_size] = { 0 };

	std::cerr << "Error in " << function << " (" << file << ", line " << line << ")";

	if (strerror_r(errno, buffer, buffer_size) != 0)
	{
		std::cerr << " - failed to decipher errno " << errno << "\n";
	}
	else
	{
		std::cerr << " - " << buffer << "\n";
	}
	std::cerr << "Therefore " << message << "\n\n";
}

void signal_handler(int signal_number) noexcept
{
	if (signal_number == SIGINT || signal_number == SIGTERM || signal_number == SIGQUIT || signal_number == SIGABRT)
	{
		std::clog << "Interrupted by signal " << signal_number << ": " << strsignal(signal_number)
			<< "\nFinishing the work and shutting the server.\n";

		terminate_thread_pool();

		exit(EXIT_SUCCESS);
	}
}

void set_signal(int signal_number, struct sigaction &sa) noexcept
{
	if (sigaction(signal_number, &sa, nullptr) == -1)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		LOG_CERROR("sigaction failed");
		std::cerr << "Failed to set handler for " << strsignal(signal_number) << "\n";
	}
}

void set_signals() noexcept
{
	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sa.sa_flags = SA_RESTART | SA_NOCLDWAIT;
	sigemptyset(&sa.sa_mask);
	set_signal(SIGINT, sa);
	set_signal(SIGHUP, sa);
	set_signal(SIGTERM, sa);
	set_signal(SIGCHLD, sa);
	set_signal(SIGQUIT, sa);
	set_signal(SIGUSR1, sa);
	set_signal(SIGUSR2, sa);
}

constexpr char log_redirector::log_file_out_name[];
constexpr char log_redirector::log_file_err_name[];
constexpr char log_redirector::log_file_log_name[];

size_t set_maximal_avaliable_limit_of_fd() noexcept
{
	struct rlimit descriptors_limit;
	if (getrlimit(RLIMIT_NOFILE, &descriptors_limit) == -1)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		LOG_CERROR("getrlimit failed");
		return 0;
	}

	rlim_t previous = descriptors_limit.rlim_cur;

	descriptors_limit.rlim_cur = descriptors_limit.rlim_max;

	if (setrlimit(RLIMIT_NOFILE, &descriptors_limit) == -1)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		LOG_CERROR("setrlimit failed");
		return previous;
	}

	return descriptors_limit.rlim_cur;
}

int get_fd_of_requested_file(const char *address)
{
	std::string full_address = server_directory;
	if (address[0] == '/')
		server_directory.pop_back();
	full_address += address;

	return open(full_address.data(), O_RDONLY);
}

time_t current_time_t()
{
	return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

std::string time_t_to_string(time_t seconds_since_epoch)
{
	struct tm time_now;
	tzset();
	struct tm *ret_val = localtime_r(&seconds_since_epoch, &time_now);
	if (ret_val != &time_now)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		LOG_CERROR("requested data-string will be empty due to fail of localtime_r");
		return "";
	}

	const char *day_of_week[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	const char *month[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	constexpr size_t date_max_length = 512;
	char date_string[date_max_length];

	const char format_string[] = ", %d  %Y %T GMT";
	size_t strftime_res = strftime(date_string, date_max_length, format_string, &time_now);
	if (strftime_res == 0)
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		LOG_CERROR("requested data-string will be empty due to fail of strftime_res");
		return "";
	}

	std::string result;
	result += day_of_week[time_now.tm_wday];
	result += date_string;
	constexpr size_t month_position = 8;
	result.insert(month_position, month[time_now.tm_mon]);

	return result;
}

void atexit_terminator() noexcept
{
	terminate_thread_pool();

	std::clog << "Exiting. " << time_t_to_string(current_time_t()) << std::endl;
}

[[noreturn]] void terminate_handler() noexcept
{
	terminate_thread_pool();
	std::clog << "Terminating at " << time_t_to_string(current_time_t()) << std::endl;

	std::exception_ptr current = std::current_exception();
	if (current)
	{
		std::clog << "Current exception: ";
		try
		{
			std::rethrow_exception(current);
		}
		catch (std::exception &e)
		{
			std::clog << e.what() << std::endl;
			std::lock_guard<std::mutex> lock(cerr_mutex);
			std::cerr << "Terminating because of unhandled exception: " << e.what() << "\n";
		}
		catch (...)
		{
			std::clog << " yet unknown to the modern science." << std::endl;
			std::lock_guard<std::mutex> lock(cerr_mutex);
			std::cerr << "Terminating because of unknown exception (not an std::exception)\n";
		}
	}
	else
	{
		std::lock_guard<std::mutex> lock(cerr_mutex);
		std::cerr << "Terminating because of unhnandled bare throw; or unprovoked call to std::terminate()\n";
	}

	std::abort();
}
