#!/bin/bash

# Report when output differs in html logic updates between versions

BINARY="./build/src/md2html"
TESTS_FILE_DIR="./tests"

failed=0

for md in "$TESTS_FILE_DIR"/*; do
	md2html -i $md -o global.html
  ./build/src/md2html -i $md -o local.html

	diff -c global.html local.html > /dev/null 2>&1
	if [ $? -eq 1 ]; then
		failed=1
	fi
done

if [ $failed -eq 1 ]; then
	exit 1
else
	exit 0
fi
