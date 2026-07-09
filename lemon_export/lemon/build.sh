#!/bin/bash
g++ -g -O2 -std=gnu++23 -static -lrt \
  -Wl,--whole-archive -lpthread -Wl,--no-whole-archive \
  Sharon.cpp -o Sharon 2>&1
