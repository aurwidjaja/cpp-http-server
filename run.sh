#!/bin/sh
#
# Build the server with CMake, then run it.
# Any arguments are passed through to the server.

set -e # Exit early if any commands fail

(
  cd "$(dirname "$0")" # Ensure compile steps are run within the repository directory
  # Only use the vcpkg toolchain if vcpkg is installed (VCPKG_ROOT set)
  cmake -B build -S . ${VCPKG_ROOT:+-DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake}
  cmake --build ./build
)

exec $(dirname "$0")/build/http-server "$@"