#!/bin/bash

make clean && make -j $(nproc) TESTS=1
sudo ./build/test
