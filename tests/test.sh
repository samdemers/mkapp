#!/bin/bash
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details at
# http://www.gnu.org/copyleft/gpl.html
#
#
# Test script. All the directories below the working directory are
# assumed to contain directories for which an executable file of the
# same name exists under ../src/mkapp. These directories contain *.in
# files which provide the standard input for the test. The executable
# must provide the output written in the corresponding *.out file for
# a test to succeed. Optional *.err files can be used to specify which
# error output is expected.
#

set -e

EXEC_DIR=../src/mkapp
LIB_DIR=../src/libmkapp

TMP=/tmp/$(basename "$0.$$")

cleanup() {
    rm -rf "$TMP"
}

trap cleanup EXIT INT TERM


mkdir -p "$TMP"

((TEST_COUNT = 0));
((ERROR_COUNT = 0));

LD_LIBRARY_PATH="$LIB_DIR"

for DIR in $(find . -maxdepth 1 -type d); do

    BASEDIR=$(basename "$DIR")
    EXEC="$EXEC_DIR/$BASEDIR"

    for IN in $(find "$DIR" -maxdepth 1 -type f -iname "*.in"); do
        BASE=$(basename "$IN" .in)
        DIR=$(dirname "$IN")
        OUT="$DIR/$BASE.out"
        ERR="$DIR/$BASE.err"
        FILE="$DIR/$BASE.file"
        FAILED=false
        ((TEST_COUNT++))

        echo -n "Running test $BASEDIR/$BASE... " >&2

        # Run the test
        if [ -e "$FILE" ]; then
            cat "$IN" | "$EXEC" "$FILE" > "$TMP/$BASE.out" 2> "$TMP/$BASE.err" \
                || FAILED=true
        else
            cat "$IN" | "$EXEC" > "$TMP/$BASE.out" 2> "$TMP/$BASE.err" \
                || FAILED=true
        fi

        # Check output
        if [ -e "$OUT" ] && ! diff "$OUT" "$TMP/$BASE.out" >/dev/null; then
            FAILED=true
            echo "failed." >&2
            echo -ne "\tExpected: \"" >&2
            cat "$OUT" >&2
            echo "\"" >&2
            echo -ne "\tGot: \"" >&2
            cat "$TMP/$BASE.out" >&2
            echo "\"" >&2
        fi

        # Check error
        if [ -e "$ERR" ] && ! diff "$ERR" "$TMP/$BASE.err" >/dev/null; then
            FAILED=true
            echo "failed." >&2
            echo -ne "\tExpected error: \"" >&2
            cat "$ERR" >&2
            echo "\"" >&2
            echo -ne "\tGot: \"" >&2
            cat "$TMP/$BASE.err" >&2
            echo "\"" >&2
        fi

        # Say whether the test has failed or not
        if $FAILED; then
            ((ERROR_COUNT++))
        else
            echo "done." >&2
        fi
    done
done

echo -e "\nTests run:\t$TEST_COUNT"
echo -e "Tests failed:\t$ERROR_COUNT"
