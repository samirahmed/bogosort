#!/bin/sh

cd lib
make clean
make

cd ../client
make clean
make

cd ../server
make clean
make
