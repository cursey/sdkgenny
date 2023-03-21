#!/usr/bin/env sh

find include src examples -type f \( -name "*.hpp" -o -name "*.cpp" \) |
while read file; do
    echo "$file"
    clang-format -i -style=file "$file"
done
