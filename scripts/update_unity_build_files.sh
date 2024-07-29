#!/bin/sh

find . -name "*.cpp" | awk '{print "#include \"" substr($0,3) "\"" }'
