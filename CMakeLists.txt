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

# Gather Source Files
file(GLOB ALL_SRCS "src/*.c")

# Sources for the main engine (everything except test files)
set(ENGINE_SRCS ${ALL_SRCS})
list(REMOVE_ITEM ENGINE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/test_search.c")
list(REMOVE_ITEM ENGINE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/test_polybook.c")

# Sources for test_search (everything except main.c and test_polybook.c)
set(TEST_SRCS ${ALL_SRCS})
list(REMOVE_ITEM TEST_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")
list(REMOVE_ITEM TEST_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/test_polybook.c")

# Sources for test_polybook (everything except main.c and test_search.c)
set(TEST_POLYBOOK_SRCS ${ALL_SRCS})
list(REMOVE_ITEM TEST_POLYBOOK_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")
list(REMOVE_ITEM TEST_POLYBOOK_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/test_search.c")

# Define the executables
add_executable(chess_engine ${ENGINE_SRCS})
add_executable(test_search ${TEST_SRCS})
add_executable(test_polybook ${TEST_POLYBOOK_SRCS})

# Include directories (Must come AFTER add_executable)
target_include_directories(chess_engine PRIVATE include)
target_include_directories(test_search PRIVATE include)
target_include_directories(test_polybook PRIVATE include)

# Output directory configuration (works on all platforms)
set_target_properties(chess_engine test_search test_polybook PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Platform-specific library linking and definitions
if(WIN32)
  # Link Winsock library for Windows sockets functions (ntohl, ntohs)
  target_link_libraries(chess_engine ws2_32)
  target_link_libraries(test_search ws2_32)
  target_link_libraries(test_polybook ws2_32)

  # Define Windows version for Winsock
  target_compile_definitions(chess_engine PRIVATE _WIN32_WINNT=0x0601)
  target_compile_definitions(test_search PRIVATE _WIN32_WINNT=0x0601)
  target_compile_definitions(test_polybook PRIVATE _WIN32_WINNT=0x0601)

  target_compile_definitions(chess_engine PRIVATE _CRT_SECURE_NO_WARNINGS)
  target_compile_definitions(test_search PRIVATE _CRT_SECURE_NO_WARNINGS)
  target_compile_definitions(test_polybook PRIVATE _CRT_SECURE_NO_WARNINGS)

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

enable_testing()
add_test(NAME test_search COMMAND test_search)
add_test(NAME test_polybook COMMAND test_polybook)
