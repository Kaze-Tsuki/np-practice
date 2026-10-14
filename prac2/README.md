# Practice 2 

Trying to scale the server up need not only IO multiplexing.

Server runs a echo function, add meow to every line.
Client is a simple echo client.

## Basic 

Using threads to utilize more core and CPU, avoiding overhead of process creation.

Includes:
- Socket Acceptor (Main)
- Thread creating function
- Worker function run in threads

```bash
cd prac2 
make thread1
./thread_basic
# on different terminal
./client
```

## Improve

Using the template from basic, but use thread pool to avoid creating too many threads.

Includes:
- Socket Acceptor
- Thread assigning function
- Thread pool class
- Worker function

```bash
cd prac2 
make thread2
./thread_improve
# on different terminal
./client
```


