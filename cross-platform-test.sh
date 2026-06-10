#!/bin/bash
# cross-platform-check.sh
# Works on Windows (Git Bash/WSL), macOS, and Linux

set -e

# Colors for output
if [ -t 1 ]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    BLUE='\033[0;34m'
    NC='\033[0m'
else
    RED=''; GREEN=''; YELLOW=''; BLUE=''; NC=''
fi

# Detect OS
detect_os() {
    case "$(uname -s)" in
        Darwin*)    echo "macos" ;;
        Linux*)     echo "linux" ;;
        MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
        *)          echo "unknown" ;;
    esac
}

OS=$(detect_os)
PROJECT_PATH="$(pwd)"
FAILED_TESTS=()
PASSED_TESTS=()

# Helper functions
print_header() {
    echo ""
    echo -e "${BLUE}=========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}=========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_failure() {
    echo -e "${RED}❌ $1${NC}"
}

print_info() {
    echo -e "${YELLOW}ℹ️ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️ $1${NC}"
}

# Safe cleanup function that handles permissions
safe_cleanup() {
    local dir=$1

    if [ -d "$dir" ]; then
        print_info "Cleaning up $dir..."

        # Try normal removal first
        if rm -rf "$dir" 2>/dev/null; then
            return 0
        fi

        # Try Docker container to clean if available (handles root-owned files from docker builds)
        if command -v docker &> /dev/null; then
            print_warning "Using docker to clean $dir..."
            docker run --rm -v "$(pwd):/project" -w /project ubuntu:22.04 rm -rf "$dir" &>/dev/null && return 0
        fi

        # If that fails, try with sudo (Linux/macOS)
        if command -v sudo &> /dev/null; then
            print_warning "Permission issues detected, using sudo to clean..."
            sudo rm -rf "$dir" 2>/dev/null && return 0
        fi

        # Last resort: change permissions then remove
        if command -v sudo &> /dev/null; then
            sudo chown -R $(whoami) "$dir" 2>/dev/null
            rm -rf "$dir" 2>/dev/null
        fi
    fi
}

# Check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check Docker availability
check_docker() {
    if command_exists docker; then
        print_success "Docker found"
        return 0
    else
        print_warning "Docker not found"
        print_info "Install Docker Desktop from: https://www.docker.com/products/docker-desktop/"
        print_info "Docker allows testing Linux containers on any OS"
        return 1
    fi
}

# Test with Docker (Linux containers) - NOW WORKS ON ALL PLATFORMS!
test_with_docker() {
    if ! command_exists docker; then
        print_info "Docker not installed - skipping container tests"
        return 0
    fi

    print_header "Testing Linux Compatibility via Docker"
    print_info "Running Linux containers on $OS (this is normal!)"

    local docker_passed=0
    local docker_failed=0

    # Test Ubuntu 22.04
    print_info "Testing Ubuntu 22.04..."
    if docker run --rm -v "$PROJECT_PATH:/project" -w /project ubuntu:22.04 bash -c "
        apt update > /dev/null 2>&1
        apt install -y cmake gcc make > /dev/null 2>&1
        rm -rf build
        mkdir build && cd build
        cmake .. > /dev/null 2>&1
        make > /dev/null 2>&1
        ./bin/test_search && ./bin/test_polybook
    " 2>/dev/null; then
        print_success "Ubuntu 22.04 test passed"
        ((docker_passed++))
    else
        print_failure "Ubuntu 22.04 test failed"
        ((docker_failed++))
    fi

    # Test Ubuntu 20.04 (older GLIBC)
    print_info "Testing Ubuntu 20.04..."
    if docker run --rm -v "$PROJECT_PATH:/project" -w /project ubuntu:20.04 bash -c "
        apt update > /dev/null 2>&1
        apt install -y cmake gcc make > /dev/null 2>&1
        rm -rf build
        mkdir build && cd build
        cmake .. > /dev/null 2>&1
        make > /dev/null 2>&1
        ./bin/test_search && ./bin/test_polybook
    " 2>/dev/null; then
        print_success "Ubuntu 20.04 test passed"
        ((docker_passed++))
    else
        print_failure "Ubuntu 20.04 test failed"
        ((docker_failed++))
    fi

    # Test with Clang compiler
    print_info "Testing Ubuntu 22.04 with Clang compiler..."
    if docker run --rm -v "$PROJECT_PATH:/project" -w /project ubuntu:22.04 bash -c "
        apt update > /dev/null 2>&1
        apt install -y cmake clang make > /dev/null 2>&1
        rm -rf build
        mkdir build && cd build
        cmake -DCMAKE_C_COMPILER=clang .. > /dev/null 2>&1
        make > /dev/null 2>&1
        ./bin/test_search && ./bin/test_polybook
    " 2>/dev/null; then
        print_success "Ubuntu 22.04 with Clang passed"
        ((docker_passed++))
    else
        print_failure "Ubuntu 22.04 with Clang failed"
        ((docker_failed++))
    fi

    if [ $docker_failed -eq 0 ]; then
        print_success "All Docker/Linux tests passed ($docker_passed tests)"
        PASSED_TESTS+=("Docker Linux containers ($docker_passed tests)")
    else
        print_failure "$docker_failed Docker tests failed"
        FAILED_TESTS+=("Docker Linux containers ($docker_failed failed)")
    fi
}

# Test Windows cross-compilation
test_windows_cross() {
    if ! command_exists docker; then
        return 0
    fi

    print_header "Windows Cross-Compilation Test"
    print_info "Building Windows .exe files using MinGW in Docker"

    # Create MinGW toolchain file
    cat > mingw-toolchain.cmake << 'EOF'
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_C_COMPILER_TARGET x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF

    print_info "Building Windows executables (can't run them on $OS, but can verify compilation)..."
    if docker run --rm -v "$PROJECT_PATH:/project" -w /project ubuntu:22.04 bash -c "
        apt update > /dev/null 2>&1
        apt install -y cmake gcc-mingw-w64-x86-64 make > /dev/null 2>&1
        rm -rf build_win
        mkdir build_win && cd build_win
        cmake -DCMAKE_TOOLCHAIN_FILE=../mingw-toolchain.cmake .. > /dev/null 2>&1
        make -j4 > /dev/null 2>&1
        [ -f bin/chess_engine.exe ] && [ -f bin/test_search.exe ] && [ -f bin/test_polybook.exe ]
    " 2>/dev/null; then
        print_success "Windows executables created successfully"
        print_info "Note: Can't run Windows .exe files on $OS, but compilation works"
        PASSED_TESTS+=("Windows cross-compilation (build only)")
    else
        print_failure "Windows cross-compilation failed"
        FAILED_TESTS+=("Windows cross-compilation")
    fi
}

# Native build on current OS
test_native_build() {
    print_header "Native Build on $OS"

    # Check CMake
    if ! command_exists cmake; then
        print_failure "CMake not found"
        print_info "Install CMake: https://cmake.org/download/"
        FAILED_TESTS+=("Native build (CMake missing)")
        return 1
    fi
    print_success "CMake found: $(cmake --version | head -1)"

    # Check compiler
    if [ "$OS" = "windows" ]; then
        if command_exists cl.exe || command_exists gcc.exe; then
            print_success "Compiler found"
        else
            print_warning "No compiler found. Install Visual Studio or MinGW"
            FAILED_TESTS+=("Native build (compiler missing)")
            return 1
        fi
    elif [ "$OS" = "macos" ]; then
        if command_exists clang || command_exists gcc; then
            print_success "Compiler found"
        else
            print_warning "No compiler found. Run: xcode-select --install"
            FAILED_TESTS+=("Native build (compiler missing)")
            return 1
        fi
    else # Linux
        if command_exists gcc || command_exists clang; then
            print_success "Compiler found"
        else
            print_warning "No compiler found. Install: sudo apt install build-essential"
            FAILED_TESTS+=("Native build (compiler missing)")
            return 1
        fi
    fi

    # Clean up old build with permission handling
    safe_cleanup "build"

    # Build and test
    print_info "Building and testing natively on $OS..."
    mkdir -p build
    cd build

    if [ "$OS" = "windows" ]; then
        cmake .. || {
            print_failure "CMake configuration failed"
            return 1
        }
        cmake --build . --config Release || {
            print_failure "Build failed"
            return 1
        }
        cd bin/Release
        ./test_search.exe && ./test_polybook.exe || {
            print_failure "Tests failed"
            return 1
        }
    else
        cmake .. || {
            print_failure "CMake configuration failed"
            return 1
        }
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) || {
            print_failure "Build failed"
            return 1
        }
        cd bin
        ./test_search && ./test_polybook || {
            print_failure "Tests failed"
            return 1
        }
    fi

    cd ../..
    print_success "Native build and tests passed on $OS"
    PASSED_TESTS+=("Native build on $OS")
    return 0
}

# Check for common portability issues
check_portability() {
    print_header "Checking Code Portability"

    local issues=0

    # Skip if no src directory
    if [ ! -d "src" ]; then
        print_warning "No src directory found, skipping portability check"
        return 0
    fi

    # Check for Windows-specific paths
    if grep -r "C:\\" src/ 2>/dev/null | grep -v "Binary" | grep -q .; then
        print_failure "Found Windows-specific paths (C:\\) in source code"
        issues=$((issues + 1))
    fi

    if grep -r "#include <pthread.h>" src/ 2>/dev/null | grep -q .; then
        print_warning "Found POSIX threads (pthread.h) - needs Windows alternative"
        issues=$((issues + 1))
    fi

    if grep -r "#include <unistd.h>" src/ 2>/dev/null | grep -q .; then
        print_warning "Found Unix-specific include (unistd.h)"
        issues=$((issues + 1))
    fi

    if [ $issues -eq 0 ]; then
        print_success "No obvious portability issues found"
    else
        print_warning "Found $issues potential portability issues"
    fi
}

# Generate final report
generate_report() {
    print_header "Test Summary Report"

    echo ""
    echo -e "${GREEN}Passed: ${#PASSED_TESTS[@]}${NC}"
    for test in "${PASSED_TESTS[@]}"; do
        echo -e "  ${GREEN}✓${NC} $test"
    done

    echo ""
    if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
        echo -e "${RED}Failed: ${#FAILED_TESTS[@]}${NC}"
        for test in "${FAILED_TESTS[@]}"; do
            echo -e "  ${RED}✗${NC} $test"
        done
        return 1
    else
        echo -e "${GREEN}All tests passed! 🎉${NC}"
        return 0
    fi
}

# Main execution
main() {
    print_header "Cross-Platform Compatibility Checker"
    echo "Detected OS: $OS"
    echo "Project: chess_engine"
    echo ""

    # Run checks
    check_portability
    test_native_build
    test_with_docker
    test_windows_cross

    # Final report
    generate_report
    exit $?
}

# Run main function
main
