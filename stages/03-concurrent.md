# Stage 3: Concurrent connections

In this stage, you'll make your server keep running and handle many clients at the same time.

Right now your server accepts one connection, answers it, and exits. A real server runs until you stop it, and it doesn't make one slow client hold up everyone else.

This stage has two parts. Do them in order: part 1 is useful by itself, and part 2 builds on it.

## Part 1: Keep serving

Your `main` currently does this once:

```
accept → recv → build response → send → close client → close server → exit
```

Change it so that everything from `accept` to closing the client happens in a loop:

```
socket → setsockopt → bind → listen        (once, at startup)

loop forever:
  accept → recv → build response → send → close client
```

The server socket (`server_fd`) stays open for the whole time the server runs. Only the client socket (`client_fd`) gets closed after each request. Each `accept` returns a new `client_fd` for the next connection.

To stop the server, press Ctrl+C in its terminal.

### Move the per-client code into a function

Before you add the loop, move everything that deals with one client into its own function:

```cpp
void handle_client(int client_fd) {
  // recv, extract_path, parse_headers, pick the response, send, close(client_fd)
}
```

Then the loop in `main` only has to call `handle_client(client_fd)` after each `accept`. This also sets you up for part 2.

### Errors no longer end the program

Inside `main`, `return 1;` exits the whole server. Once the per-client code is in `handle_client`, a failure while talking to one client (say, `recv` returning `0` because the client hung up) must only end that one connection. Close `client_fd` and `return;` from `handle_client`. The server keeps running for everyone else.

The same goes for `accept`: if it fails in the loop, print an error and `continue` to the next pass instead of returning from `main`.

## Part 2: Handle clients at the same time

After part 1, the server answers requests one after another. That works until a client is slow. While the server is inside `recv` waiting for one client to send its request, it can't `accept` anyone else. Every other client waits in line.

You can see this for yourself before you start part 2 (see "Tests" below): open a connection with `nc` and don't send anything. Your server will sit in `recv` waiting for it, and a `curl` in another terminal will hang.

The fix is to handle each client on its own **thread**. A thread is a separate path of execution that runs at the same time as the rest of your program. The main thread does nothing but `accept` connections, and each connection gets a new thread that runs `handle_client`:

```
main thread:     accept → start thread → accept → start thread → accept → ...
                               │                        │
thread 1:                      └─ handle_client(fd 4)   │
thread 2:                                               └─ handle_client(fd 5)
```

A slow client now only blocks its own thread.

### Starting a thread

Threads come from `<thread>`. Creating a `std::thread` starts running a function right away:

```cpp
std::thread t(handle_client, client_fd);   // runs handle_client(client_fd) on a new thread
```

The first argument is the function to run. The rest are the arguments to pass it.

After creating it, you have to decide what happens to the `std::thread` object. There are two choices:

- `t.join()` waits until the thread finishes. That would block the main thread until the client is done, which is exactly the problem you're trying to fix.
- `t.detach()` lets the thread run on its own. The main thread moves on right away, and the thread cleans up after itself when `handle_client` returns.

Use `detach()`. If you do neither, your program crashes: when a `std::thread` object is destroyed while still joinable, the C++ runtime calls `std::terminate`.

Your `CMakeLists.txt` already links the threads library (`Threads::Threads`), so there's nothing to change there.

## Tests

Run your server with `./run.sh`. From now on, you don't restart it between requests.

### Part 1: Several requests in a row

Run these one after another without restarting the server:

| Request | Expected status | Expected body |
|---|---|---|
| `curl -i http://localhost:4221/` | `200 OK` | *(empty)* |
| `curl -i http://localhost:4221/echo/abc` | `200 OK` | `abc` |
| `curl -i -A "foobar/1.2.3" http://localhost:4221/user-agent` | `200 OK` | `foobar/1.2.3` (Content-Length: 12) |
| `curl -i http://localhost:4221/nope` | `404 Not Found` | *(empty)* |

The server must still be running after all four.

### Part 2: A slow client doesn't block others

Use three terminals.

1. **Terminal 1:** start the server with `./run.sh`.
2. **Terminal 2:** open a connection and don't type anything:
   ```sh
   nc -c localhost 4221
   ```
   `nc` (netcat) opens a raw TCP connection and sends whatever you type. `-c` makes it end lines with `\r\n`, like HTTP requires. Leave it sitting there.
3. **Terminal 3:** while `nc` is still connected, run:
   ```sh
   curl -i http://localhost:4221/echo/abc
   ```
   You must get the `200 OK` response with body `abc` right away.

   **Without threads**, this `curl` hangs, because your server is stuck in `recv` waiting for the `nc` client. That's the test failing. Try it after part 1 to see the problem before you fix it.

4. **Back in terminal 2:** type `GET /echo/hi HTTP/1.1` and press Enter twice. The second Enter sends the empty line that ends the request. `nc` should print a `200 OK` response with body `hi`. The waiting connection still gets its answer.

### Part 2: Many at once

This starts 5 requests at the same time:

```sh
for i in 1 2 3 4 5; do curl -s http://localhost:4221/echo/$i & done; wait; echo
```

All five bodies (`1` through `5`) must be printed. They can come back in any order, because the threads finish in whatever order they happen to finish. Running in parallel means the order isn't fixed.

## Notes

- **Pass `client_fd` by value.** `std::thread t(handle_client, client_fd)` copies the number into the new thread. That's what you want: the main thread immediately goes back to `accept` and overwrites its own `client_fd` with the next connection. If the thread shared that variable instead of having its own copy, it could end up reading the *next* client's number.
- **Each thread closes its own socket.** `close(client_fd)` belongs at the end of `handle_client`, not in `main`. If `main` closed it right after starting the thread, the thread would be trying to read from a closed socket.
- **Don't close `server_fd` in `handle_client`.** It's shared by the whole server. Closing it would stop all future connections.
- **Make sure every exit from `handle_client` closes the socket.** Every `return` in it should come after a `close(client_fd)`. Each socket that's never closed stays open for as long as the server runs, and a process can only have a limited number open at once.
- **Log output may get mixed up.** Several threads printing with `std::cout` at the same time can have their lines interleaved. That's harmless for now.
- **A client that hangs up early can kill the server.** If a client disconnects before your `send`, the operating system sends your process a `SIGPIPE` signal, and by default that ends the whole program, every thread included. Add `signal(SIGPIPE, SIG_IGN);` at the top of `main` (from `<csignal>`). With that, `send` just returns `-1` for that one client instead.
- **Port 4221 already in use?** If `./run.sh` says it can't bind, an old copy of the server is probably still running in another terminal. Stop it with Ctrl+C there.
