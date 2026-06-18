cmake_minimum_required(VERSION 3.10)

# Project Name
project(chess_engine LANGUAGES C)

# Set the C standard (optional, but recommended)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Cross-platform compiler flags
if(MSVC)
  # Windows / MSVC
  add_compile_options(/W4)
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  # Linux, macOS, MinGW, etc.
  add_compile_options(-Wall -Wextra)

  # Platform-specific flags
  if(APPLE)
    # macOS specific flags
    add_compile_options(-mmacosx-version-min=10.12)
  elseif(UNIX AND NOT APPLE)
    # Linux specific flags
    add_compile_options(-D_GNU_SOURCE)
  endif()
endif()

# Gather Engine Source Files
file(GLOB ENGINE_SRCS "src/*.c")

# Define the main engine executable
add_executable(chess_engine ${ENGINE_SRCS})

# Include directories (Must come AFTER add_executable)
target_include_directories(chess_engine PRIVATE include)

# Output directory configuration (works on all platforms)
set_target_properties(chess_engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Platform-specific library linking and definitions
if(WIN32)
  # Link Winsock library for Windows sockets functions (ntohl, ntohs)
  target_link_libraries(chess_engine ws2_32)

  # Define Windows version for Winsock
  target_compile_definitions(chess_engine PRIVATE _WIN32_WINNT=0x0601)
  target_compile_definitions(chess_engine PRIVATE _CRT_SECURE_NO_WARNINGS)

  message(STATUS "Windows detected - linking ws2_32.lib")
elseif(APPLE)
  message(STATUS "macOS detected")
elseif(UNIX)
  message(STATUS "Linux/Unix detected")
endif()

# Optional: Set debug/release flags for better cross-platform behavior
if(NOT MSVC)
  # For GCC/Clang - add debug symbols or optimization
  set(CMAKE_C_FLAGS_DEBUG "-g -O0")
  set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# Print platform information (helpful for debugging)
message(STATUS "Building on: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER_ID}")

# Enable CTest testing
enable_testing()

# Discover and configure all test files in the tests/ directory
file(GLOB TEST_FILES "tests/*.c")

# Test engine sources configuration (exclude main.c so we can link test mains)
set(TEST_ENGINE_SRCS ${ENGINE_SRCS})
list(REMOVE_ITEM TEST_ENGINE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")

foreach(test_file ${TEST_FILES})
    # Extract filename without directory path and extension
    get_filename_component(test_name ${test_file} NAME_WE)
    
    # Define the test executable
    add_executable(${test_name} ${test_file} ${TEST_ENGINE_SRCS})
    
    # Include directories (Must come AFTER add_executable)
    target_include_directories(${test_name} PRIVATE include)
    
    # Output directory configuration (works on all platforms)
    set_target_properties(${test_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    
    # Platform-specific library linking and definitions for tests
    if(WIN32)
      target_link_libraries(${test_name} ws2_32)
      target_compile_definitions(${test_name} PRIVATE _WIN32_WINNT=0x0601)
      target_compile_definitions(${test_name} PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
    
    # Add target to test suite
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
