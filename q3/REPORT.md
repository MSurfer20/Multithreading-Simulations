# Question - 3
## How to Run
```
cd q3
make
./client -> Run the client
./server [NUMER OF THREAD POOLS] -> Run the server
```
## SERVER
* The server runs on the port 12000 by default. This value is specified in `q3/headers.h`.
* The server has a `BACKLOG` of 2048 which is specified in `server.cpp`. Thus, the server can listen to a maximum of 2048 connections.
* Now, the code initialized the dictinary as empty for keys until 10000, and also initialized mutex locks for the dictinary elements.
* A queue is also created in which all incoming connections get enqueued, along with a lock for it, as well as a condition variable to remove busy waiting.
* Now, the server starts threads that keep looking for work to do in the queue.
* The code then binds a socket and starts listening to it, and accepts new connections in an infinite loop.
* Whenever a new connection comes, it is enqueued into the connection queue after acquiring the lock of the queue.
* It then broadcasts a signal to the `pool_cond` variable to tell the threads to start looking for work in the queue.
### Thread Pool
* In the pool thread's function, we first acquire the lock of the queue from which we want to dequeue the connection.
* Then we start a while loop till the queue is empty and inside we wait on the condition variable of the queue.
* After getting some connection, the condition variable is released and the thread passes the connection into another function that processes and communicates with the connection.
### Connection Handler
* This function handles the connection, and reads data from the connection file descriptor, performs the required operations, and then writes the results back into the connection fd.
## CLIENT
* Each request has its own thread.
* First, the thread sleeps for the designated time.
* Then, it binds a new socket to connect with the socket of the server.
* The client thread then tries to connect to the server.
* Upon a successful connection, it sends the commands that it is supposed to run, and then waits for the response.
* Before doing `cout` anywhere in the client, the code locks a mutex lock so that the print statements of different threads don't overlap with each other.