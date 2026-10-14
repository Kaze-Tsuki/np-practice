#include <cstdlib>
#include <memory.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

const char *ADDRESS = "127.0.0.1";
const unsigned int PORT = 8000;

void handleClient(int connfd) {
  // simple echo server, add meow to every line.
  char buffer[1024] = {0};
  int valread;
  while ((valread = read(connfd, buffer, 1024 - 1 - 5)) > 0) {
    write(STDOUT_FILENO, buffer, valread);
    write(STDOUT_FILENO, "\n", 1);
    memcpy(buffer + valread, " meow\n", 7);
    int valsend = send(connfd, buffer, valread + 5, 0);
    if (valsend < 0) {
      perror("send failed");
      break;
    }
  }

  if (valread == -1) {
    char tmp[300];
    sprintf(tmp, "Client %d connection error\n", connfd);
    write(STDOUT_FILENO, tmp, strlen(tmp));
    close(connfd);
    return;
  }
  close(connfd);
  return;
}

void handleConnection(int connfd) {
  std::thread t{handleClient, connfd};
  t.detach();
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

  while (1) {
    int connfd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (connfd < 0) {
      perror("Accept failed");
      continue;
    }
    printf("Client %d connected\n", connfd);
    handleConnection(connfd);
  }

  close(server_fd);

  return 0;
}
