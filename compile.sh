#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <filename.c>"
    exit 1
fi

filename="$1"

# Check if the file exists
if [ ! -f "$filename" ]; then
    echo "File not found: $filename"
    exit 1
fi

# Check if the file has a .c extension
extension="${filename##*.}"
if [ "$extension" != "c" ]; then
    echo "Invalid file extension: $extension"
    exit 1
fi

# Compile the file and capture the output
output=$(gcc -o /dev/null -Wall -Wextra "$filename" 2>&1)

# Count the number of errors and warnings
errors=$(echo "$output" | grep -c "error:")
warnings=$(echo "$output" | grep -c "warning:")

# Print the results
echo "Errors: $errors"
echo "Warnings: $warnings"
