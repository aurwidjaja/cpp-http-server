# Stage 1: Respond with body

In this stage, you'll implement the `/echo/{str}` endpoint, which accepts a string and returns it in the response body.

## Response body

A response body is used to return content to the client. This content may be an entire web page, a file, a string, or anything else that can be represented with bytes.

Your `/echo/{str}` endpoint must return a `200` response, with the response body set to the given string, and with a `Content-Type` and `Content-Length` header.

Here's an example of an `/echo/{str}` request:

```
GET /echo/abc HTTP/1.1\r\nHost: localhost:4221\r\nUser-Agent: curl/7.64.1\r\nAccept: */*\r\n\r\n
```

And here's the expected response:

```
HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 3\r\n\r\nabc
```

Here's a breakdown of the response:

```
// Status line
HTTP/1.1 200 OK
\r\n                          // CRLF that marks the end of the status line

// Headers
Content-Type: text/plain\r\n  // Header that specifies the format of the response body
Content-Length: 3\r\n         // Header that specifies the size of the response body, in bytes
\r\n                          // CRLF that marks the end of the headers

// Response body
abc                           // The string from the request
```

The two headers are required for the client to be able to parse the response body. Note that each header ends in a CRLF, and the entire header section also ends in a CRLF.

## Tests

Run your server with `./run.sh`, then in another terminal:

```sh
curl -i http://localhost:4221/echo/abc
```

Your server must respond with:

- Status code `200`
- `Content-Type` header set to `text/plain`
- `Content-Length` header set to the length of the given string
- Response body set to the given string

Expected output:

```
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 3

abc
```

Also verify that the earlier behavior still works (restart the server between requests, since it handles one request and exits):

| Request | Expected status | Expected body |
|---|---|---|
| `curl -i http://localhost:4221/echo/abc` | `200 OK` | `abc` |
| `curl -i http://localhost:4221/echo/hello123` | `200 OK` | `hello123` (Content-Length: 8) |
| `curl -i http://localhost:4221/` | `200 OK` | *(empty)* |
| `curl -i http://localhost:4221/abc` | `404 Not Found` | *(empty)* |

## Notes

- The `{str}` part is everything after `/echo/` in the path. Check whether the path **starts with** `/echo/` using `std::string::starts_with` (C++20), and get the rest with `substr` called with one argument: `substr(start)` takes everything from `start` to the end, like Python's `s[start:]`. For example, `std::string("/echo/abc").substr(6)` is `"abc"`, because `"/echo/"` is 6 characters.
- `Content-Length` is the number of **bytes** in the body. For a `std::string`, that's `.size()`.
- To put a number into a string, use `std::to_string(n)`. `"Content-Length: " + n` compiles but produces garbage: it does pointer arithmetic on the string literal.
- Strings are joined with `+`, like in Python. At least one side must be a `std::string`: `"a" + "b"` (two literals) won't compile, but `std::string("a") + "b"` will.
- The body is **not** followed by `\r\n`. `Content-Length` tells the client exactly where it ends.
- Paths like `/echo/hello%20world` are percent-encoded (`%20` = space). You don't need to decode them in this stage; echo them back as-is.
