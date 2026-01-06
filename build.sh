#!/bin/bash
set -e  # Exit on any error

# Default to Debug build
BUILD_TYPE="Debug"
CLEAN_BUILD=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [-d|--debug] [-r|--release] [-c|--clean] [-h|--help]"
            echo "  -d, --debug    Build in Debug mode (default)"
            echo "  -r, --release  Build in Release mode"
            echo "  -c, --clean    Clean build directory before building"
            echo "  -h, --help     Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

echo "Building SFML project in $BUILD_TYPE mode..."

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ] && [ -d "build" ]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

# Change to build directory
cd build

# Run cmake configuration
echo "Running CMake configuration..."
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

# Build the project
echo "Building project in $BUILD_TYPE mode..."
make -j$(nproc)

echo "Build completed successfully in $BUILD_TYPE mode!"