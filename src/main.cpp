#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

// const so that since we're passing in a pointer it won't modify the value
// passed in
std::string extract_path(const std::string& request_body) {
  size_t first_space = request_body.find(' ');  // search from the beginning
  size_t second_space = request_body.find(' ', first_space + 1);
  return request_body.substr(first_space + 1, second_space - first_space - 1);
}

int main(int argc, char** argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // TODO: Uncomment the code below to pass the first stage
  // SOCK_STREAM = ordered stream of data since HTTPS servers
  // are built on top of TCP ... SOCK_DGRAM = UDP
  // AF_INET = IPv4 ... VS AF_INET6 = IPv6
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }
  //
  // // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // // ensures that we don't run into 'Address already in use' errors
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

  int client_fd = accept(server_fd, (struct sockaddr*)&client_addr,
                         (socklen_t*)&client_addr_len);
  if (client_fd < 0) {
    std::cerr << "Failed to create client socket\n";
    return 1;
  }

  std::cout << "Client connected\n";
  char buffer[4096];
  ssize_t len_req_bytes = recv(client_fd, &buffer, sizeof(buffer), 0);
  if (len_req_bytes <= 0) {
    std::cerr << "Failed to receive client request\n";
    return 1;
  }

  std::string request_body(buffer, len_req_bytes);
  std::string url_path = extract_path(request_body);
  const std::string not_found_msg = "HTTP/1.1 404 Not Found\r\n\r\n";
  const std::string success_msg = "HTTP/1.1 200 OK\r\n\r\n";

  std::string msg;
  if (url_path == "/") {
    std::string msg = success_msg;
  } else {
    std::string msg = not_found_msg;
  }
  ssize_t sent = send(client_fd, msg.c_str(), msg.size(), 0);

  close(client_fd);
  close(server_fd);

  return 0;
}
