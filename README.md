# CPP_server
C++ server using thread pool

[Initially,](https://github.com/Bolbot/mailru) this was written as the exam of the C++ Multithreading course by Mail.ru Group to finish the course and [get the certificate](https://stepik.org/cert/157416).
Since then it was slightly rewritten to better adhere to standards of [good writing](https://en.wikipedia.org/wiki/Code_Complete) 
and overall prefer somewhat more modern tools and approaches such as boost::program_options, noexcept functions, and RAII.
Also, some optional features such as HTTP/0.9 backward compatibility and some statuses other than 200 and 404 were provided.

## Requirements

* Linux. It's written for Linux only and strongly relies on UNIX networking headers such as `<sys/socket.h>`
* C++11 compliant compiler
* Boost

## How to use

Compile and run `final` executable with the following arguments:
* `host` or `h` is for IP of your host, say 127.0.0.1
* `port` or `p` is the port this server opens for listening (use any in range 1024..65535)
* `directory` or `d` is the directory in your filesystem that the server will treat as root

Every parameter is mandatory, can be specified both in short or long form. For example:
```
final -h 127.0.0.1 -p 11111 -d /tmp/
```

From here on server daemonizes and uses redirected `std::cout`, `std::cerr`, and `std::clog` to write logs.
It listens to the specified port and accepts incoming connections. Accepted requests are processed and statuses are returned according to [HTTP/1.0](https://www.w3.org/Protocols/HTTP/1.0/spec.html).
Supported statuses are:
* 200 - OK
* 400 - Bad Request
* 404 - Not Found
* 405 - Method Not Allowed
* 414 - URI Too Long
* 500 - Internal Server Error
* 505 - HTTP Version Not Supported

If the request was valid and the requested file exists, it is returned along with success status. Otherwise, just the status is returned.
Based on failure reason this can be one of the client errors such as Bad Request or [HTTP/1.1](https://www.w3.org/Protocols/rfc2616/rfc2616.html) URI Too Long
or one of the server errors like HTTP Version Not Supported.

To stop the server you can use one of the following signals
* SIGINT
* SIGTERM
* SIGQUIT
* SIGABRT

## Under the hood

This server is multithreaded. It uses thread pools with work-stealing queues as described in [Concurrency in Action by Anthony Williams](https://www.bogotobogo.com/cplusplus/files/CplusplusConcurrencyInAction_PracticalMultithreading.pdf).  
For parsing the requests [`std::regex`](https://en.cppreference.com/w/cpp/regex) is applied.  
The obsolete CMake version was the Mail.ru requirement. CMake part will be rewritten in one of the upcoming updates.






