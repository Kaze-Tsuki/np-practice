#include <arpa/inet.h>
#include <cstdlib>
#include <netinet/in.h>
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

int main() {
  int status, valread, client_fd;
  struct sockaddr_in serv_addr;
  const char *hello = "hello from client";
  char buffer[1024] = {0};
  if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket create failed");
    return -1;
  }

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_family = AF_INET;

  if (inet_pton(AF_INET, ADDRESS, &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address\n");
    return -1;
  }

  if ((status = connect(client_fd, (struct sockaddr *)&serv_addr,
                        sizeof(serv_addr))) < 0) {
    printf("Connection failer\n");
    return -1;
  }

  valread = read(client_fd, buffer, 1024 - 1);
  const char *CONN_EST = "Connection established";
  if (strcmp(CONN_EST, buffer) != 0) {
    perror("missing server connection info");
    exit(EXIT_FAILURE);
  }

  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGPIPE, &sa, NULL) < 0) {
    perror("sigaction create failed");
  }

  struct pollfd fds[2];
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;
  fds[1].fd = client_fd;
  fds[1].events = POLLIN;
  int poll_result;

  while (1) {
    poll_result = poll(fds, 2, -1);
    if (poll_result <= 0)
      continue;

    if (fds[0].revents & POLLIN) {
      valread = read(STDIN_FILENO, buffer, 1024 - 1);
      buffer[valread] = '\0';
      send(client_fd, buffer, valread, 0);
    }

    // server closed
    if (fds[1].revents & POLLHUP) {
      printf("Server close connection.\nExit...\n");
      break;
    }

    if (fds[1].revents & POLLIN) {
      valread = read(client_fd, buffer, 1024 - 1);
      if (valread == 0) {
        printf("Server disconnect\n");
        break;
      }
      write(STDOUT_FILENO, buffer, valread);
    }
  }

  close(client_fd);

  return 0;
}
