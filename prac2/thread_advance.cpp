#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <memory.h>
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

const char *ADDRESS = "127.0.0.1";
const unsigned int PORT = 8000;

struct Task {
  std::function<void(int)> function;
  int param;
  Task(std::function<void(int)> f, int p) : function(f), param(p) {};
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
    while (true) {
      Task t{[](int) {}, 0};
      {
        std::unique_lock<std::mutex> lock{mtx};

        // task appear or need to stop
        cv.wait(lock, [this]() { return !tasks.empty() || stop_flag.load(); });

        if (stop_flag.load() && tasks.empty()) {
          // safe quiting
          return;
        }

        // acquire task and pop
        t = std::move(tasks.front());
        tasks.pop();
      }
      // execute after release lock
      t.function(t.param);
    }
  }

public:
  ThreadPool(int size) : pool_size(size) {
    if (pool_size <= 0)
      throw("[ThreadPool] Assign invalid pool size");
    pool.reserve(pool_size);
    for (int i = 0; i < pool_size; i++) {
      pool.emplace_back(&ThreadPool::workerLoop, this);
    }
  }
  void addTask(std::function<void(int)> f, int p) {
    {
      std::lock_guard<std::mutex> lock(mtx);
      tasks.push((Task){f, p});
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
  static ThreadPool tp{3};
  tp.addTask(handleClient, connfd);
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
