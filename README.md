# cpp-http-server

An HTTP/1.1 server written from scratch in C++, using POSIX sockets directly. Project guided by Claude so spec written out in stages under `stages` dir.

## Requirements

- A C++23 compiler (Apple clang or a recent clang/gcc)
- CMake 3.13+

## Build and run

```sh
./run.sh
```

This builds the server into `build/` and starts it on port 4221.

## Stages

The server is built up in stages. Each one has a spec in [`stages/`](stages/):

1. [Respond with body](stages/01-echo.md): `/echo/{str}`

The project started from the CodeCrafters "Build Your Own HTTP Server" starter code.
