# Stage 2: Read a header

In this stage, you'll implement the `/user-agent` endpoint, which reads the `User-Agent` request header and returns it in the response body.

So far your server has only looked at the request line (the first line). This stage is about parsing the **headers** that come after it.

## Request headers

The client sends headers to give the server extra information about the request. They come right after the request line, one per line, and the header section ends with an empty line.

Here's an example of a `/user-agent` request:

```
GET /user-agent HTTP/1.1\r\nHost: localhost:4221\r\nUser-Agent: foobar/1.2.3\r\nAccept: */*\r\n\r\n
```

Here's a breakdown of the request:

```
// Request line
GET                          // HTTP method
/user-agent                  // Request target (the path)
HTTP/1.1                     // HTTP version
\r\n                         // CRLF that marks the end of the request line

// Headers
Host: localhost:4221\r\n     // Header that specifies the server's host and port
User-Agent: foobar/1.2.3\r\n // Header that describes the client's software
Accept: */*\r\n              // Header that specifies which media types the client accepts
\r\n                         // CRLF that marks the end of the headers

// Request body (empty)
```

Each header has the form `Name: value`. The name and value are separated by a colon, usually followed by a single space.

Header names are **case-insensitive**. `User-Agent`, `user-agent` and `USER-AGENT` are all the same header, and your server must treat them that way.

## Response

Your `/user-agent` endpoint must return a `200` response, with the response body set to the value of the `User-Agent` header, and with a `Content-Type` and `Content-Length` header. This is the same response format as `/echo/{str}`.

For the request above, the expected response is:

```
HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nfoobar/1.2.3
```

Note that the body is just the value, `foobar/1.2.3`. It doesn't include the header name, the colon or the space after it.

## Tests

Run your server with `./run.sh`, then in another terminal:

```sh
curl -i -A "foobar/1.2.3" http://localhost:4221/user-agent
```

`-A` sets the `User-Agent` header that curl sends. Without it, curl sends its own (for example `curl/8.7.1`), which depends on which version you have installed.

Your server must respond with:

- Status code `200`
- `Content-Type` header set to `text/plain`
- `Content-Length` header set to the length of the `User-Agent` value
- Response body set to the `User-Agent` value

Expected output:

```
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 12

foobar/1.2.3
```

Also verify these cases (restart the server between requests, since it still handles one request and exits):

| Request | Expected status | Expected body |
|---|---|---|
| `curl -i -A "foobar/1.2.3" http://localhost:4221/user-agent` | `200 OK` | `foobar/1.2.3` (Content-Length: 12) |
| `curl -i http://localhost:4221/user-agent` | `200 OK` | your curl's default, e.g. `curl/8.7.1` (Content-Length: 10) |
| `curl -i -H "user-agent: lower/1.0" http://localhost:4221/user-agent` | `200 OK` | `lower/1.0` (Content-Length: 9) |
| `curl -i http://localhost:4221/echo/abc` | `200 OK` | `abc` |
| `curl -i http://localhost:4221/` | `200 OK` | *(empty)* |
| `curl -i http://localhost:4221/abc` | `404 Not Found` | *(empty)* |

The third row sends the header name in lowercase. It checks that your name comparison is case-insensitive.

## Notes

- **Splitting into lines.** Every line ends in `\r\n`, not just `\n`. Use `find("\r\n", pos)` to locate the end of each line, and `substr` to cut it out. Start right after the request line and stop when you reach an empty line (two `\r\n` in a row). `find` returns `std::string::npos` when there's no match, so check for that or you'll loop forever or read past the end.
- **Splitting a header.** Find the first `:` in the line with `find(':')`. The name is everything before it, the value is everything after it. Use the **first** colon only, because values can contain colons too: in `Host: localhost:4221` the value is `localhost:4221`.
- **Trimming the value.** The value usually starts with a space. Skip any spaces or tabs at the start (and the end, to be safe) before using it. `find_first_not_of(" \t")` and `find_last_not_of(" \t")` give you the positions to `substr` between.
- **Case-insensitive names.** The simplest way is to lowercase the name before comparing it to `"user-agent"`. C++ has no built-in `lower()` on strings, so loop over the characters and call `std::tolower` on each one (from `<cctype>`). Pass it `static_cast<unsigned char>(c)`, not a plain `char`: `std::tolower` has undefined behavior for negative values, which a `char` can be.
- **Store the headers in a map.** Rather than searching for `User-Agent` directly, consider parsing *all* the headers into a `std::unordered_map<std::string, std::string>` (from `<unordered_map>`), with lowercased names as keys. It works like a Python `dict`: `headers["user-agent"] = value;` to store, and `headers.find("user-agent")` to look up. Later stages will need other headers, and this will save you rewriting the parser.
- **Watch out with `headers["key"]`.** Reading a missing key with `[]` doesn't raise an error like Python does. It silently inserts an empty string and returns it. To check whether a key exists, use `headers.contains("user-agent")` (C++20) or compare `headers.find("user-agent")` against `headers.end()`.
- **Reuse your response code.** `/user-agent` and `/echo/{str}` return the same kind of response with a different body. This is a good moment to pull that into a function, something like `std::string text_response(const std::string& body)`.
- **Exact path match.** Match `/user-agent` with `==`, not `starts_with`. `/user-agent/extra` should still be a `404`.

## Stretch goal (optional)

If the request has no `User-Agent` header at all, respond with `400 Bad Request` and an empty body instead of crashing or returning garbage. You can test this with `curl -i -H "User-Agent:" http://localhost:4221/user-agent`. A header with nothing after the colon tells curl to leave that header out of the request.
