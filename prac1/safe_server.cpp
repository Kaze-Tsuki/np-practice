#include <csignal>
#include <cstdlib>
#include <memory.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

const char *ADDRESS = "127.0.0.1";
const unsigned int PORT = 8000;

void setKeepAlive(int sock_fd, int idle_time = 5, int interval = 3,
                  int times = 3) {
  static int keepalive = 1;
  static int keepidle = 5;
  static int keepintvl = 3;
  static int keepcnt = 3;

  keepidle = idle_time;
  keepintvl = interval;
  keepcnt = times;

  setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
  setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));
  setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl,
             sizeof(keepintvl));
  setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt));
}

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

  setKeepAlive(client1_fd);
  setKeepAlive(client2_fd);

  const char *CONN_EST = "Connection established";
  send(client1_fd, CONN_EST, strlen(CONN_EST), 0);
  send(client2_fd, CONN_EST, strlen(CONN_EST), 0);

  struct pollfd client_input[2];
  client_input[0].fd = client1_fd;
  client_input[0].events = POLLIN;

  client_input[1].fd = client2_fd;
  client_input[1].events = POLLIN;

  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGPIPE, &sa, NULL) < 0) {
    perror("sigaction create failed");
  }

  int poll_result;
  bool is_up = true;
  while (is_up) {
    poll_result = poll(client_input, 2, -1);

    // No events, then skip
    if (poll_result <= 0)
      continue;

    for (int i = 0; i < 2; i++) {
      if (client_input[i].revents & POLLHUP) {
        printf("Client disconnect\n");
        is_up = false;
        break;
      }
      // pollin event detected for input fd i
      if (client_input[i].revents & POLLIN) {
        // get the message and send to the other side
        valread = read(client_input[i].fd, buffer, 1024 - 1);
        // Clint closed
        if (valread == 0) {
          printf("Client close connection\nExit...\n");
          is_up = false;
          break;
        }
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
