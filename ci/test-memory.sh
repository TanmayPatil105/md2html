#!/bin/bash

BINARY="./build/src/md2html"
TESTS_FILE_DIR="./tests"

failed=0

for md in "$TESTS_FILE_DIR"/*; do
	valgrind --leak-check=full --error-exitcode=1 $BINARY -i $md
	if [ $? -eq 1 ]; then
		failed=1
	fi
done

if [ $failed -eq 1 ]; then
	exit 1
else
	exit 0
fi
