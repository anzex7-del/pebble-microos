#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
cc -Wall -Wextra -Werror -framework JavaScriptCore tests/js_runner.c -o build/js_runner
./build/js_runner tests/browser_mock.js preview/app.js tests/browser_test.js
