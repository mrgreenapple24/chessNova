#!/bin/bash

# build.sh - Cross-platform build script for chess_engine
# Works on Windows (Git Bash), macOS, and Linux

set -e  # Exit on error

# Colors for output (only if terminal supports it)
if [ -t 1 ]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    BLUE='\033[0;34m'
    NC='\033[0m' # No Color
else
    RED=''
    GREEN=''
    YELLOW=''
    BLUE=''
    NC=''
fi

# Print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Detect OS
detect_os() {
    case "$(uname -s)" in
        CYGWIN*|MINGW32*|MSYS*|MINGW*)
            OS="windows"
            print_info "Detected Windows (Git Bash/MSYS2)"
            ;;
        Darwin*)
            OS="macos"
            print_info "Detected macOS"
            ;;
        Linux*)
            OS="linux"
            print_info "Detected Linux"
            ;;
        *)
            OS="unknown"
            print_warning "Unknown operating system: $(uname -s)"
            ;;
    esac
}

# Check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check build requirements
check_requirements() {
    local missing_deps=0

    # Check for CMake
    if ! command_exists cmake; then
        print_error "CMake is not installed!"
        echo "Please install CMake:"
        if [ "$OS" = "windows" ]; then
            echo "  - Download from https://cmake.org/download/"
            echo "  - Or use: choco install cmake (if Chocolatey is installed)"
            echo "  - Or use: winget install Kitware.CMake"
        elif [ "$OS" = "macos" ]; then
            echo "  - brew install cmake"
        elif [ "$OS" = "linux" ]; then
            echo "  - Ubuntu/Debian: sudo apt install cmake"
            echo "  - Fedora: sudo dnf install cmake"
            echo "  - Arch: sudo pacman -S cmake"
        fi
        missing_deps=$((missing_deps + 1))
    fi

    # Check for C compiler
    if ! command_exists gcc && ! command_exists clang && ! command_exists cc; then
        print_error "No C compiler found!"
        echo "Please install a C compiler:"
        if [ "$OS" = "windows" ]; then
            echo "  - Install MinGW-w64 or Visual Studio Build Tools"
            echo "  - Or use: choco install mingw"
        elif [ "$OS" = "macos" ]; then
            echo "  - xcode-select --install"
            echo "  - Or: brew install gcc"
        elif [ "$OS" = "linux" ]; then
            echo "  - Ubuntu/Debian: sudo apt install build-essential"
            echo "  - Fedora: sudo dnf install gcc"
            echo "  - Arch: sudo pacman -S gcc"
        fi
        missing_deps=$((missing_deps + 1))
    fi

    if [ $missing_deps -gt 0 ]; then
        exit 1
    fi

    print_success "All build requirements satisfied"
}

# Clean build directory
clean() {
    if [ -d "build" ]; then
        print_info "Cleaning build directory..."
        rm -rf build
        print_success "Clean complete"
    else
        print_info "No build directory to clean"
    fi
}

# Configure with CMake
configure() {
    print_info "Configuring project with CMake..."

    # Create build directory if it doesn't exist
    mkdir -p build
    cd build

    # Configure based on OS
    if [ "$OS" = "windows" ]; then
        # For Windows, specify generator if needed
        if command_exists ninja; then
            print_info "Using Ninja generator"
            cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
        else
            print_info "Using default generator (MinGW Makefiles or Visual Studio)"
            cmake .. -DCMAKE_BUILD_TYPE=Release
        fi
    else
        # Unix-like systems (macOS, Linux)
        cmake .. -DCMAKE_BUILD_TYPE=Release
    fi

    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed"
        cd ..
        exit 1
    fi

    cd ..
    print_success "CMake configuration complete"
}

# Build the project
build() {
    print_info "Building project..."

    cd build

    # Determine number of parallel jobs
    if [ "$OS" = "macos" ] || [ "$OS" = "linux" ]; then
        JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
        print_info "Using $JOBS parallel jobs"
        cmake --build . --parallel $JOBS
    else
        # Windows
        cmake --build . --config Release --parallel
    fi

    if [ $? -ne 0 ]; then
        print_error "Build failed"
        cd ..
        exit 1
    fi

    cd ..
    print_success "Build complete"
}

run_tests() {
    print_info "Running tests..."

    local test_search_bin=""
    local test_polybook_bin=""

    if [ -f "build/bin/test_search" ]; then
        test_search_bin="./build/bin/test_search"
    elif [ -f "build/bin/Release/test_search.exe" ]; then
        test_search_bin="./build/bin/Release/test_search.exe"
    elif [ -f "build/bin/test_search.exe" ]; then
        test_search_bin="./build/bin/test_search.exe"
    fi

    if [ -f "build/bin/test_polybook" ]; then
        test_polybook_bin="./build/bin/test_polybook"
    elif [ -f "build/bin/Release/test_polybook.exe" ]; then
        test_polybook_bin="./build/bin/Release/test_polybook.exe"
    elif [ -f "build/bin/test_polybook.exe" ]; then
        test_polybook_bin="./build/bin/test_polybook.exe"
    fi

    if [ -n "$test_search_bin" ] && [ -n "$test_polybook_bin" ]; then
        print_info "Running search tests..."
        $test_search_bin
        if [ $? -ne 0 ]; then
            print_error "Search tests failed"
            exit 1
        fi

        print_info "Running polybook and polyglot tests..."
        $test_polybook_bin
        if [ $? -ne 0 ]; then
            print_error "Polybook/Polyglot tests failed"
            exit 1
        fi

        print_success "All tests passed"
    else
        print_warning "Test executables not found. Building first..."
        build
        run_tests
    fi
}

# Run the main engine
run_engine() {
    print_info "Running chess engine..."

    if [ -f "build/bin/chess_engine" ] || [ -f "build/bin/Release/chess_engine.exe" ] || [ -f "build/bin/chess_engine.exe" ]; then
        if [ -f "build/bin/chess_engine" ]; then
            ./build/bin/chess_engine
        elif [ -f "build/bin/Release/chess_engine.exe" ]; then
            ./build/bin/Release/chess_engine.exe
        elif [ -f "build/bin/chess_engine.exe" ]; then
            ./build/bin/chess_engine.exe
        fi
    else
        print_warning "Engine executable not found. Building first..."
        build
        run_engine
    fi
}

# Print usage information
usage() {
    echo "Usage: $0 [OPTION]"
    echo "Build script for chess_engine (cross-platform)"
    echo ""
    echo "Options:"
    echo "  configure  - Only run CMake configuration"
    echo "  build      - Build the project (default)"
    echo "  clean      - Clean build directory"
    echo "  test       - Build and run tests"
    echo "  run        - Build and run the chess engine"
    echo "  all        - Clean, configure, build, and test"
    echo "  help       - Show this help message"
    echo ""
    echo "Examples:"
    echo "  ./build.sh          # Build the project"
    echo "  ./build.sh test     # Run tests"
    echo "  ./build.sh run      # Run the engine"
    echo "  ./build.sh clean    # Clean build files"
}

# Main function
main() {
    # Check if we're in the right directory
    if [ ! -f "CMakeLists.txt" ]; then
        print_error "CMakeLists.txt not found!"
        echo "Please run this script from the project root directory"
        exit 1
    fi

    # Detect OS
    detect_os

    # Parse command line argument
    case "${1:-build}" in
        configure)
            check_requirements
            configure
            ;;
        build)
            check_requirements
            configure
            build
            print_success "Build finished! Executables are in build/bin/"
            ;;
        clean)
            clean
            ;;
        test)
            check_requirements
            configure
            build
            run_tests
            ;;
        run)
            check_requirements
            configure
            build
            run_engine
            ;;
        all)
            clean
            check_requirements
            configure
            build
            run_tests
            print_success "All tasks completed successfully!"
            ;;
        help|--help|-h)
            usage
            ;;
        *)
            print_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
}

# Make script executable if it's not already
if [ ! -x "$0" ]; then
    chmod +x "$0"
fi

# Run main function
main "$@"
