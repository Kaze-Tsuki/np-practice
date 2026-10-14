#include <arpa/inet.h>
#include <cstdlib>
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

  while (1) {
    valread = read(STDIN_FILENO, buffer, 1024 - 1);
    buffer[valread - 1] = '\0';
    write(client_fd, buffer, valread - 1);

    valread = read(client_fd, buffer, 1024 - 1);
    buffer[valread] = '\0';
    printf("%s\n", buffer);
  }

  close(client_fd);

  return 0;
}
