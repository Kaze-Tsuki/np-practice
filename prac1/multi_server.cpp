#include <cstdlib>
#include <memory.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

const char *ADDRESS = "127.0.0.1";
const unsigned int PORT = 8000;

int main() {
  int server_fd;
  ssize_t valread;
  struct sockaddr_in address;
  int opt = 1;
  socklen_t addrlen = sizeof(address);
  char buffer[1024] = {0};
  char input[1024] = {0};

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed\n");
    exit(EXIT_FAILURE);
  }

  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind error");
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 2) < 0) {
    perror("listen failed");
    exit(EXIT_FAILURE);
  }

  int client1_fd, client2_fd;

  if ((client1_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) <
      0) {
    perror("client 1 accept failed");
    exit(EXIT_FAILURE);
  }

  if ((client2_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) <
      0) {
    perror("client2 accept failed");
    exit(EXIT_FAILURE);
  }

  const char *CONN_EST = "Connection established";
  send(client1_fd, CONN_EST, strlen(CONN_EST), 0);
  send(client2_fd, CONN_EST, strlen(CONN_EST), 0);

  struct pollfd client_input[2];
  client_input[0].fd = client1_fd;
  client_input[0].events = POLLIN;

  client_input[1].fd = client2_fd;
  client_input[1].events = POLLIN;

  int poll_result;
  while (1) {
    poll_result = poll(client_input, 2, -1);

    // No events, then skip
    if (poll_result <= 0)
      continue;

    for (int i = 0; i < 2; i++) {
      // pollin event detected for input fd i
      if (client_input[i].revents & POLLIN) {
        // get the message and send to the other side
        valread = read(client_input[i].fd, buffer, 1024 - 1);
        send(client_input[i ^ 1].fd, buffer, valread, 0);
        write(STDOUT_FILENO, buffer, strlen(buffer));
      }
    }
  }

  close(client1_fd);
  close(client2_fd);

  close(server_fd);

  return 0;
}
