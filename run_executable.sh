#!/bin/bash

# Check if an executable file is provided as an argument
if [ $# -ne 1 ]; then
    echo "Usage: $0 <executable_file>"
    exit 1
fi

executable="$1"
total_runtime=0

# Run the executable 50 times
for ((i=1; i<=50; i++)); do
    echo "Running iteration $i..."
    start_time=$(date +%s.%N)
    ./"$executable"
    end_time=$(date +%s.%N)
    runtime=$(echo "$end_time - $start_time" | bc)
    total_runtime=$(echo "$total_runtime + $runtime" | bc)
    echo "Runtime for iteration $i: $runtime seconds"
done

# Calculate the average runtime
average_runtime=$(echo "$total_runtime *1000 / 50" | bc)

echo "Average runtime over 50 iterations: $average_runtime milliseconds"
