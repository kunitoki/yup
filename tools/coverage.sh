#!/bin/bash

# ==============================================================================
#
#   This file is part of the YUP library.
#   Copyright (c) 2025 - kunitoki@gmail.com
#
#   YUP is an open source library subject to open-source licensing.
#
#   The code included in this file is provided under the terms of the ISC license
#   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
#   to use, copy, modify, and/or distribute this software for any purpose with or
#   without fee is hereby granted provided that the above copyright notice and
#   this permission notice appear in all copies.
#
#   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
#   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
#   DISCLAIMED.
#
# ==============================================================================

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build"

echo -e "${BLUE}YUP Coverage Report Generator${NC}"
echo "=================================="

# Check if lcov is installed
if ! command -v lcov &> /dev/null; then
    echo -e "${RED}Error: lcov is not installed.${NC}"
    echo "On Ubuntu/Debian: sudo apt-get install lcov"
    echo "On macOS: brew install lcov"
    exit 1
fi

# Check if genhtml is installed
if ! command -v genhtml &> /dev/null; then
    echo -e "${YELLOW}Warning: genhtml is not installed. HTML reports will not be generated.${NC}"
fi

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${BLUE}Creating build directory...${NC}"
    mkdir -p "$BUILD_DIR"
fi

# Configure with coverage enabled
echo -e "${BLUE}Configuring CMake with coverage enabled...${NC}"
cd "$BUILD_DIR"
cmake "$PROJECT_ROOT" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DYUP_BUILD_TESTS=ON \
    -DYUP_BUILD_EXAMPLES=OFF \
    -DYUP_ENABLE_COVERAGE=ON

# Build tests
echo -e "${BLUE}Building tests...${NC}"
cmake --build . --target yup_tests --parallel $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Clean previous coverage data
echo -e "${BLUE}Cleaning previous coverage data...${NC}"
if [ -f "${BUILD_DIR}/coverage/coverage.info" ]; then
    lcov --directory . --zerocounters
fi

# Run tests
echo -e "${BLUE}Running tests...${NC}"
cd "${BUILD_DIR}/tests/Debug" || cd "${BUILD_DIR}/tests"
if [ -f "./yup_tests" ]; then
    ./yup_tests
else
    echo -e "${RED}Error: yup_tests executable not found${NC}"
    exit 1
fi

cd "$BUILD_DIR"

# Generate coverage reports
echo -e "${BLUE}Generating coverage reports...${NC}"
mkdir -p coverage

# Capture coverage data
lcov --directory . --capture --output-file coverage/coverage.info

# Remove unwanted files from coverage
lcov --remove coverage/coverage.info \
    "*/thirdparty/*" \
    "*/build/*" \
    "*/tests/*" \
    "*/examples/*" \
    "*/usr/*" \
    "*/opt/*" \
    "*/.conan/*" \
    "*/CMakeFiles/*" \
    --output-file coverage/coverage_final.info

# Generate per-module reports
modules=("yup_core" "yup_events" "yup_audio_basics" "yup_audio_devices" "yup_audio_processors" "yup_graphics" "yup_gui")

echo -e "${BLUE}Generating per-module coverage reports...${NC}"
for module in "${modules[@]}"; do
    echo -e "${YELLOW}Processing module: $module${NC}"
    mkdir -p "coverage/$module"

    # Extract coverage for this specific module
    lcov --extract coverage/coverage_final.info "*/modules/$module/*" \
        --output-file "coverage/$module/coverage.info"

    # Show summary for this module
    if [ -s "coverage/$module/coverage.info" ]; then
        echo -e "${GREEN}Coverage summary for $module:${NC}"
        lcov --list "coverage/$module/coverage.info"

        # Generate HTML report if genhtml is available
        if command -v genhtml &> /dev/null; then
            genhtml "coverage/$module/coverage.info" \
                --output-directory "coverage/$module/html" \
                --title "YUP $module Coverage Report" \
                --show-details \
                --legend
        fi
    else
        echo -e "${YELLOW}No coverage data found for $module${NC}"
    fi
done

# Generate combined HTML report
if command -v genhtml &> /dev/null; then
    echo -e "${BLUE}Generating combined HTML coverage report...${NC}"
    genhtml coverage/coverage_final.info \
        --output-directory coverage/html \
        --title "YUP Combined Coverage Report" \
        --show-details \
        --legend
fi

# Display final summary
echo -e "${GREEN}Coverage reports generated successfully!${NC}"
echo "=================================="
echo -e "${BLUE}Coverage summary:${NC}"
lcov --list coverage/coverage_final.info

if command -v genhtml &> /dev/null; then
    echo ""
    echo -e "${GREEN}HTML reports available at:${NC}"
    echo "  Combined: ${BUILD_DIR}/coverage/html/index.html"
    for module in "${modules[@]}"; do
        if [ -f "${BUILD_DIR}/coverage/$module/html/index.html" ]; then
            echo "  $module: ${BUILD_DIR}/coverage/$module/html/index.html"
        fi
    done
fi

echo ""
echo -e "${BLUE}Coverage files:${NC}"
echo "  Combined: ${BUILD_DIR}/coverage/coverage_final.info"
for module in "${modules[@]}"; do
    if [ -f "${BUILD_DIR}/coverage/$module/coverage.info" ]; then
        echo "  $module: ${BUILD_DIR}/coverage/$module/coverage.info"
    fi
done