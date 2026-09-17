cmake_minimum_required(VERSION 3.20)

# --- Project ---
project(chess_engine
    VERSION 1.0.0
    DESCRIPTION "A chess engine written in C"
    LANGUAGES C
)

# --- C standard ---
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# --- Export compile_commands.json for clangd/LSP ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Default build type when none specified ---
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

# --- Test toggle (must be declared before any enable_testing / add_test) ---
option(BUILD_TESTING "Build tests" OFF)

# ============================================================
#  Helper: configure a target with platform-specific settings
# ============================================================
function(configure_chess_target target_name)
    # --- Warnings ---
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target_name} PRIVATE -Wall -Wextra)

        if(APPLE)
            target_compile_options(${target_name} PRIVATE
                -mmacosx-version-min=10.12)
        elseif(UNIX)
            target_compile_options(${target_name} PRIVATE -D_GNU_SOURCE)
        endif()
    endif()

    # --- Platform-specific linking & defines ---
    if(WIN32)
        target_link_libraries(${target_name} PRIVATE ws2_32)
        target_compile_definitions(${target_name} PRIVATE
            _WIN32_WINNT=0x0601
            _CRT_SECURE_NO_WARNINGS
        )
    endif()

    # --- Output directory (all platforms) ---
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
endfunction()

# ============================================================
#  Debug/Release flags for GCC/Clang
# ============================================================
if(NOT MSVC)
    set(CMAKE_C_FLAGS_DEBUG   "-g -O0")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# ============================================================
#  Engine core library (everything except main.c)
# ============================================================
file(GLOB ENGINE_SRCS CONFIGURE_DEPENDS "src/*.c")

set(ENGINE_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")
list(REMOVE_ITEM ENGINE_SRCS "${ENGINE_MAIN}")

add_library(chess_core STATIC ${ENGINE_SRCS})
target_include_directories(chess_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
configure_chess_target(chess_core)

# ============================================================
#  Main executable
# ============================================================
add_executable(chess_engine "${ENGINE_MAIN}")
target_link_libraries(chess_engine PRIVATE chess_core)
configure_chess_target(chess_engine)

# ============================================================
#  Tests (opt-in via -DBUILD_TESTING=ON)
# ============================================================
if(BUILD_TESTING)
    enable_testing()

    file(GLOB TEST_FILES CONFIGURE_DEPENDS "tests/*.c")

    foreach(test_file ${TEST_FILES})
        get_filename_component(test_name ${test_file} NAME_WE)

        add_executable(${test_name} ${test_file})
        target_link_libraries(${test_name} PRIVATE chess_core)
        configure_chess_target(${test_name})

        # Link Unity if present (see submodule section above)
        if(TARGET unity)
            target_link_libraries(${test_name} PRIVATE unity)
        endif()

        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

# ============================================================
#  Custom targets (usable via `cmake --build ... --target X`)
# ============================================================
find_program(CLANG_FORMAT_EXE clang-format)

if(CLANG_FORMAT_EXE)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
            ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c
        COMMENT "Formatting C sources with clang-format"
        VERBATIM
    )
else()
    add_custom_target(format
        COMMAND ${CMAKE_COMMAND} -E echo
            "clang-format not found — install it to use 'make format'"
    )
endif()

add_custom_target(run
    COMMAND $<TARGET_FILE:chess_engine>
    DEPENDS chess_engine
    COMMENT "Running chess_engine"
    USES_TERMINAL
)

# ============================================================
#  Status output
# ============================================================
message(STATUS "")
message(STATUS "===== chess_engine configuration =====")
message(STATUS "Build type     : ${CMAKE_BUILD_TYPE}")
message(STATUS "System         : ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler       : ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "Source dir     : ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Binary dir     : ${CMAKE_BINARY_DIR}")
message(STATUS "Output bin dir : ${CMAKE_BINARY_DIR}/bin")
message(STATUS "Tests enabled  : ${BUILD_TESTING}")
message(STATUS "Engine sources : ${ENGINE_SRCS}")
if(BUILD_TESTING)
    message(STATUS "Test files     : ${TEST_FILES}")
endif()
if(WIN32)
    message(STATUS "Platform libs  : ws2_32")
endif()
message(STATUS "=======================================")
message(STATUS "")