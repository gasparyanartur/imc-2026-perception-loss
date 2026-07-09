#!/bin/bash
# Build script for lemon v30 (and lime v9 base algorithm)
# Matches Kattis compile flags: g++-14 -g -O2 -std=gnu++23 -static
g++ -g -O2 -std=gnu++23 -static -lrt \
  -Wl,--whole-archive -lpthread -Wl,--no-whole-archive \
  Sharon.cpp -o Sharon 2>&1
