#include <cstdlib>
#include <memory.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

  int client1_fd;

  if ((client1_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) <
      0) {
    perror("client 1 accept failed");
    exit(EXIT_FAILURE);
  }

  valread = read(client1_fd, buffer, 1024 - 1);

  printf("Server recv: %s\n", buffer);
  send(client1_fd, "hello client", strlen("hello client"), 0);
  printf("Server: hello sent\n");

  close(client1_fd);

  close(server_fd);

  return 0;
}
