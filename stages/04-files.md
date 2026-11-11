# Stage 4: Return a file

In this stage, you'll add a `/files/{filename}` route that sends back the contents of a file from a folder on your computer.

You tell the server which folder to use when you start it:

```sh
./run.sh --directory /tmp/
```

`run.sh` passes any arguments straight through to your server, so they show up in `main`'s `argv`.

## What the route does

```
GET /files/foo
```

- If `/tmp/foo` **exists**, respond with `200 OK` and the file's contents as the body:
  ```
  HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: 14\r\n\r\nHello, World!\n
  ```
- If it **doesn't exist**, respond with `404 Not Found`, the same as for an unknown route.

`application/octet-stream` means "raw bytes". It tells the client not to assume the file is text, because it could be an image or anything else.

## Step 1: Read `--directory` in `main`

`main(int argc, char** argv)` receives the command-line arguments:

```
./run.sh --directory /tmp/

argc = 3
argv[0] = "build/http-server"   (the program itself)
argv[1] = "--directory"
argv[2] = "/tmp/"
```

Loop over `argv` starting at index 1. When you find `"--directory"`, the next item is the folder. Save it in a `std::string`.

## Step 2: Pass the folder to `handle_client`

`handle_client` needs to know the folder, so add a second parameter:

```cpp
void handle_client(int client_fd, std::string directory)
```

`std::thread` passes extra arguments along in order:

```cpp
std::thread t(handle_client, client_fd, directory);
```

Each thread gets its own copy of `directory`, the same way it gets its own `client_fd`.

## Step 3: Add the `/files/` route

Inside `handle_client`, add a branch for paths that start with `/files/`:

1. **Get the filename.** It's everything after `/files/`, just like `/echo/` in stage 1.
2. **Build the full path.** That's `directory + filename`, so `"/tmp/" + "foo"` gives `"/tmp/foo"`.
3. **Open the file.** If it fails to open, send a 404.
4. **Read the whole file into a string,** then send it with the headers above.

Reading files is the part C++ makes awkward, so here it is. It needs `#include <fstream>` and `#include <sstream>`:

```cpp
std::ifstream file(path, std::ios::binary);   // open the file
if (!file) {
  // couldn't open it (doesn't exist) → 404
}
std::stringstream ss;
ss << file.rdbuf();                             // copy the whole file into ss
std::string contents = ss.str();                // as a std::string
```

`std::ios::binary` reads the bytes exactly as they are, so the server can send images and other non-text files too.

## Tests

Start the server with a folder:

```sh
./run.sh --directory /tmp/
```

In another terminal, make a test file and request it:

```sh
echo 'Hello, World!' > /tmp/foo
curl -i http://localhost:4221/files/foo
```

Expected output:

```
HTTP/1.1 200 OK
Content-Type: application/octet-stream
Content-Length: 14

Hello, World!
```

The length is 14 and not 13 because `echo` adds a newline at the end of the file.

| Request | Expected status | Expected body |
|---|---|---|
| `curl -i http://localhost:4221/files/foo` | `200 OK` | `Hello, World!` (Content-Length: 14) |
| `curl -i http://localhost:4221/files/does_not_exist` | `404 Not Found` | *(empty)* |
| `curl -i http://localhost:4221/echo/abc` | `200 OK` | `abc` |
| `curl -i -A "foobar/1.2.3" http://localhost:4221/user-agent` | `200 OK` | `foobar/1.2.3` |

## Notes

- **Missing slash.** `./run.sh --directory /tmp` (no trailing `/`) would make the path `"/tmpfoo"`. If `directory` doesn't end in `/`, add one before joining. `directory.ends_with('/')` checks for it.
- **No `--directory` given.** If the server starts without the flag, `/files/` requests should just get a 404. Don't crash.
- **Content-Length is the byte count.** `contents.size()` works for any file, text or not.

## Stretch goal (optional): don't let clients escape the folder

Try this request:

```sh
curl -i --path-as-is http://localhost:4221/files/../../etc/hosts
```

(`--path-as-is` stops curl from cleaning up the `../` before sending.)

If your server sends back your Mac's `/etc/hosts`, anyone who can reach your server can read any file on your computer. This bug is called **path traversal**. The simplest fix is to reject any filename that contains `..` with a `404` or `400 Bad Request`.
