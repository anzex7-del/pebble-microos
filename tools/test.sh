#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -g -Ifirmware/include firmware/src/pebble.c tests/core_test.c -o build/core_test
./build/core_test
