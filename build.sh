#!/bin/bash
echo "Building..."
cd build
./premake5 gmake
cd ..
make
echo "Run ./bin/Debug/Minesweeper to play!"
