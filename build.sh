#!/bin/sh

set -xe

mkdir -p bin
cc -ggdb -Wall -Wextra -o bin/trang src/lexer.c src/audio.c src/parser.c src/main.c -lm -lsndfile

