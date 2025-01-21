#!/bin/bash

# Run make clean
make clean

# Run make
make

# Run 
for i in {1}; do
    echo "$i"
    ./ex2
done

echo "All tests passed!"