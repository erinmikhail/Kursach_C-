#pragma once

#include <string>

namespace network {

bool send_all(int socket_fd, const std::string& data);
bool send_line(int socket_fd, const std::string& line);
bool read_line(int socket_fd, std::string& line, size_t max_length = 1024 * 1024);
int connect_tcp(const std::string& host, int port, int timeout_ms = 1000);

} // namespace network
