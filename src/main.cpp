#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
// const so that since we're passing in a pointer it won't modify the value
// passed in
std::string extract_path(const std::string& request_body) {
  size_t first_space = request_body.find(' ');  // search from the beginning
  size_t second_space = request_body.find(' ', first_space + 1);
  return request_body.substr(first_space + 1, second_space - first_space - 1);
}
std::string to_lower(std::string s) {
  for (char& ch : s) {
    ch = std::tolower(static_cast<unsigned char>(ch));
  }
  return s;
}

std::string trim(const std::string& s) {
  size_t first = s.find_first_not_of(" \t");
  if (first == std::string::npos) return "";
  size_t last = s.find_last_not_of(" \t");
  return s.substr(first, last - first + 1);
}

std::unordered_map<std::string, std::string> parse_headers(
    const std::string& request) {
  std::unordered_map<std::string, std::string> headers;
  size_t pos = request.find("\r\n");
  if (pos == std::string::npos) {
    return headers;
  }
  pos += 2;
  while (true) {
    size_t line_end = request.find("\r\n", pos);
    if (line_end == std::string::npos) {
      return headers;
    }
    if (line_end == pos) {
      return headers;
    }
    std::string line = request.substr(pos, line_end - pos);
    size_t colon = line.find(':');
    if (colon != std::string::npos) {
      std::string name = to_lower(line.substr(0, colon));
      std::string value = trim(line.substr(colon + 1));
      headers[name] = value;
    }
    pos = line_end + 2;
  }
}
// Only call this when url_path starts with "/echo/"
std::string get_echo_body(const std::string& url_path) {
  constexpr std::string_view key = "/echo/";
  return url_path.substr(key.size());
}
int handle_client(int client_fd) {
  std::cout << "Client connected\n";
  char buffer[4096];
  ssize_t len_req_bytes = recv(client_fd, &buffer, sizeof(buffer), 0);
  if (len_req_bytes <= 0) {
    std::cerr << "Failed to receive client request\n";
    close(client_fd);
    return 1;
  }

  std::string request_body(buffer, len_req_bytes);
  auto headers = parse_headers(request_body);  // add this
  std::string url_path = extract_path(request_body);
  const std::string not_found_msg = "HTTP/1.1 404 Not Found\r\n\r\n";
  const std::string ok_msg = "HTTP/1.1 200 OK\r\n\r\n";
  constexpr std::string_view echo_fmt =
      "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
      "{}\r\n\r\n{}";
  std::string msg;
  if (url_path == "/") {
    msg = ok_msg;
  } else if (url_path.starts_with("/echo/")) {
    std::string body = get_echo_body(url_path);
    msg = std::format(echo_fmt, body.size(), body);
  } else if (url_path == "/user-agent") {
    std::string body = headers["user-agent"];
    msg = std::format(echo_fmt, body.size(), body);
  } else {
    msg = not_found_msg;
  }

  ssize_t sent = send(client_fd, msg.c_str(), msg.size(), 0);
  close(client_fd);
  return 0;
}
int main(int argc, char** argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::cout << "Starting server on port 4221\n";

  // SOCK_STREAM = ordered stream of data since HTTPS servers
  // are built on top of TCP ... SOCK_DGRAM = UDP
  // AF_INET = IPv4 ... VS AF_INET6 = IPv6
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }
  // SO_REUSEADDR lets us restart the server right away, without
  // 'Address already in use' errors from old connections in TIME_WAIT
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) !=
      0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";
  while (1) {
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr,
                           (socklen_t*)&client_addr_len);
    if (client_fd < 0) {
      std::cerr << "Failed to create client socket\n";
      return 1;
    }
    handle_client(client_fd);
  }
  close(server_fd);

  return 0;
}
