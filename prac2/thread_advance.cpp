#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <memory.h>
#include <mutex>
#include <netinet/in.h>
#include <poll.h>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

const char *ADDRESS = "127.0.0.1";
const unsigned int PORT = 8000;

struct Task {
  int connfd;
  Task(int p) : connfd(p) {};
};

class ThreadPool {
private:
  std::vector<std::thread> pool;
  int pool_size;
  std::queue<Task> tasks;
  std::mutex mtx;
  std::condition_variable cv;
  std::atomic<bool> stop_flag{false};

  void workerLoop() {
    std::vector<struct pollfd> fds;
    while (true) {
      int connfd;
      {
        connfd = 0;
        std::unique_lock<std::mutex> lock{mtx};
        if (fds.empty()) {
          cv.wait(lock,
                  [this]() { return !tasks.empty() || stop_flag.load(); });
          if (stop_flag.load() && tasks.empty()) {
            break;
          }
        }
        if (!tasks.empty()) {
          connfd = tasks.front().connfd;
          tasks.pop();
        }
      }

      // add to fds
      if (connfd > 0)
        fds.push_back({connfd, POLLIN, 0});

      int ret = poll(fds.data(), fds.size(), 1000);
      if (ret < 0) {
        perror("Thread poll -1");
        break;
      }
      if (ret == 0)
        continue;

      char buffer[1024] = {0};
      for (int i = 0; i < fds.size(); ++i) {
        auto &fd = fds.at(i);
        if (fd.revents & POLLIN) {
          int valread = read(fd.fd, buffer, 1017);
          if (valread <= 0) {
            close(fd.fd);
            fds.erase(fds.begin() + i);
            i--;
            continue;
          }
          write(STDOUT_FILENO, buffer, valread);
          write(STDOUT_FILENO, "\n", 1);
          memcpy(buffer + valread, " meow\n", 7);
          int valsend = send(fd.fd, buffer, valread + 5, 0);
          if (valsend < 0) {
            close(fd.fd);
            fds.erase(fds.begin() + i);
            i--;
            continue;
          }
        }
      }
    }
  }

public:
  int max_connection = 2;
  ThreadPool(int size) : pool_size(size) {
    if (pool_size <= 0)
      throw("[ThreadPool] Assign invalid pool size");
    pool.reserve(pool_size);
    for (int i = 0; i < pool_size; i++) {
      pool.emplace_back(&ThreadPool::workerLoop, this);
    }
  }
  void addTask(int p) {
    {
      std::lock_guard<std::mutex> lock(mtx);
      tasks.push((Task){p});
    }
    cv.notify_one();
  }
  void shutdown() {
    {
      std::lock_guard<std::mutex> lock{mtx};
      stop_flag.store(true);
    }
    cv.notify_all();

    for (std::thread &th : pool) {
      if (th.joinable())
        th.join();
    }
  }
};

void handleConnection(int connfd) {
  static ThreadPool tp{2};
  tp.addTask(connfd);
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
