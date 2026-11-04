# cpp-http-server

An HTTP/1.1 server written from scratch in C++, using POSIX sockets directly.

## Requirements

- A C++23 compiler (Apple clang or a recent clang/gcc)
- CMake 3.13+

## Build and run

```sh
./run.sh
```

This builds the server into `build/` and starts it on port 4221.

## Try it

In another terminal:

```sh
curl -i http://localhost:4221/            # 200 OK
curl -i http://localhost:4221/echo/abc    # 200 OK, body "abc"
curl -i http://localhost:4221/nope        # 404 Not Found
```

## Routes

| Route | Response |
|---|---|
| `GET /` | `200 OK`, no body |
| `GET /echo/{str}` | `200 OK`, `text/plain` body containing `{str}` |
| anything else | `404 Not Found` |

## Stages

The server is built up in stages. Each one has a spec in [`stages/`](stages/):

1. [Respond with body](stages/01-echo.md): `/echo/{str}`

The project started from the CodeCrafters "Build Your Own HTTP Server" starter code.
