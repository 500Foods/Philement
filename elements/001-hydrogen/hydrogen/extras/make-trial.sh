#!/bin/bash

cd cmake && cmake -S . -B ../build --preset default && cmake --build ../build --preset default && cmake --build ../build --preset trial && cd ..
