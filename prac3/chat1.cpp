#include <atomic>
#include <concepts>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <memory.h>
#include <memory>
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
#include <type_traits>
#include <unistd.h>
#include <vector>

const char *ADDRESS = "127.0.0.1";
const unsigned int PORT = 8000;

// Forward Declare
class EventQueue;

struct Task {
  int connfd;
  Task(int p) : connfd(p) {};
};

enum class WorkerReplyType : uint8_t {
  NO_REPLY = 0,
  REPLY_ONE = 1,
  REPLY_ALL = 2
};

template <typename F>
concept WorkerFuncConcept =
    requires(const char *ibuf, char *obuf, size_t strlen, size_t buf_size) {
      {
        F::exec(ibuf, obuf, strlen, buf_size)
      } -> std::same_as<WorkerReplyType>;
    };

template <typename F>
concept RoomWorkerConcept = requires(F t) {
  { t.run() } -> std::same_as<void>;
  { t.stop() } -> std::same_as<void>;
  { t.get_queue() } -> std::same_as<EventQueue &>;
};

struct meowFunc {
  static WorkerReplyType exec(const char *input, char *output, size_t strlen,
                              size_t buf_size) {
    write(STDOUT_FILENO, input, strlen);
    write(STDOUT_FILENO, "\n", 1);
    memcpy(output, input, strlen);
    memcpy(output + strlen, " meow\n", 7);
    return WorkerReplyType::REPLY_ALL;
  }
};

class EventQueue {
private:
  std::queue<Task> tasks;
  std::mutex mtx;
  std::condition_variable cv;

public:
  Task get(std::atomic<bool> &stop_flag) {
    int connfd;
    {
      connfd = 0;
      std::unique_lock<std::mutex> lock{mtx};

      cv.wait(lock, [this, &stop_flag]() {
        return !tasks.empty() || stop_flag.load();
      });
      if (stop_flag.load() && tasks.empty()) {
        return Task{-1};
      }

      if (!tasks.empty()) {
        connfd = tasks.front().connfd;
        tasks.pop();
      }
    }
    return Task{connfd};
  }
  void put(Task t) {
    {
      std::lock_guard<std::mutex> lock(mtx);
      tasks.push(t);
    }
    cv.notify_one();
  }
  bool is_empty() { return tasks.empty(); }
};

template <WorkerFuncConcept F> class PollRoom {
private:
  std::atomic<bool> _stop_flag;
  EventQueue _ev_qu;

public:
  PollRoom() : _stop_flag(false) {};
  EventQueue &get_queue() { return _ev_qu; }

  void stop() { _stop_flag.store(true); }
  void run() {
    std::vector<struct pollfd> fds;
    while (true) {
      if (!_ev_qu.is_empty()) {
        Task t = _ev_qu.get(_stop_flag);
        if (t.connfd == -1)
          break;
        if (t.connfd > 0) {
          fds.push_back({t.connfd, POLLIN, 0});
        }
      }
      // handle poll
      int ret = poll(fds.data(), fds.size(), 1000);
      if (ret < 0) {
        perror("Thread poll -1");
        break;
      }
      if (ret == 0)
        continue;

      char inbuffer[1024] = {0}, oubuffer[1024] = {0};
      for (int i = 0; i < fds.size(); ++i) {
        auto &fd = fds.at(i);
        if (fd.revents & POLLIN) {
          int valread = read(fd.fd, inbuffer, 1017);
          if (valread <= 0) {
            close(fd.fd);
            fds.erase(fds.begin() + i);
            i--;
            continue;
          }
          // injected function
          WorkerReplyType wrt = F::exec(inbuffer, oubuffer, valread, 1024);
          if (wrt == WorkerReplyType::REPLY_ONE)
            send(fd.fd, oubuffer, valread + 5, 0);
          else if (wrt == WorkerReplyType::REPLY_ALL)
            for (int k = 0; k < fds.size(); ++k)
              send(fds[k].fd, oubuffer, valread + 5, 0);
        }
      }
    }
  }
};

template <RoomWorkerConcept W> class ThreadPool {
private:
  std::vector<std::thread> pool;
  int pool_size;
  std::vector<std::unique_ptr<W>> rooms;

public:
  int max_connection = 2;
  ThreadPool(int size) : pool_size(size) {
    if (pool_size <= 0)
      throw("[ThreadPool] Assign invalid pool size");
    pool.reserve(pool_size);
    rooms.reserve(pool_size);
    for (int i = 0; i < pool_size; i++) {
      rooms.emplace_back(std::make_unique<W>());
      pool.emplace_back([this, i]() { rooms[i]->run(); });
    }
  }

  int pushConnection(int room, int fd) {
    if (room >= pool_size || room < 0)
      return -1;
    rooms[room]->get_queue().put(Task{fd});
    return 0;
  }

  void shutdown() {
    for (auto w : rooms) {
      w->stop();
    }

    for (std::thread &th : pool) {
      if (th.joinable())
        th.join();
    }
  }
};

void handleConnection(int connfd) {
  static ThreadPool<PollRoom<meowFunc>> tp{2};
  static int x = 0;
  // pre create 2 chat room
  tp.pushConnection(x, connfd);
  x = (x + 1) % 2;
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
