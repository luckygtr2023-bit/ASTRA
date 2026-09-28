cmake_minimum_required(VERSION 3.16)
project(AstraCosmos CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---------------------------------------------------------------------------
# Vulkan headers — vendored clone under libs/ (TDD-sanctioned fallback when
# no system Vulkan is available). No system Vulkan required.
# ---------------------------------------------------------------------------
set(VULKAN_HEADERS_DIR ${CMAKE_SOURCE_DIR}/libs/Vulkan-Headers/include)
if(NOT EXISTS ${VULKAN_HEADERS_DIR}/vulkan/vulkan.h)
  message(WARNING
    "Vulkan headers not found at ${VULKAN_HEADERS_DIR}. "
    "git clone https://github.com/KhronosGroup/Vulkan-Headers "
    "libs/Vulkan-Headers   (see docs/PHASE0_EXIT_REPORT.md)")
endif()

# ---------------------------------------------------------------------------
# Shaders: GLSL -> SPIR-V via glslangValidator, BEFORE the C++ build.
# Accepts either binary name (glslangValidator, or the renamed 'glslang'
# produced by the glslang-standalone target of the standalone CMake build).
# ---------------------------------------------------------------------------
find_program(GLSLANG_VALIDATOR NAMES glslangValidator glslang)
if(NOT GLSLANG_VALIDATOR)
  message(FATAL_ERROR
    "glslangValidator not found on PATH. Build glslang from "
    "https://github.com/KhronosGroup/glslang (target glslang-standalone) "
    "or install your distribution's glslangValidator.")
endif()

set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
set(SHADER_SVPS
    ${SHADER_OUT_DIR}/triangle.vert.spv
    ${SHADER_OUT_DIR}/triangle.frag.spv)

add_custom_command(OUTPUT ${SHADER_OUT_DIR}/triangle.vert.spv
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
  COMMAND ${GLSLANG_VALIDATOR} -V -o ${SHADER_OUT_DIR}/triangle.vert.spv
          ${CMAKE_SOURCE_DIR}/shaders/triangle.vert
  DEPENDS ${CMAKE_SOURCE_DIR}/shaders/triangle.vert
  COMMENT "glslang: triangle.vert -> SPIR-V")

add_custom_command(OUTPUT ${SHADER_OUT_DIR}/triangle.frag.spv
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
  COMMAND ${GLSLANG_VALIDATOR} -V -o ${SHADER_OUT_DIR}/triangle.frag.spv
          ${CMAKE_SOURCE_DIR}/shaders/triangle.frag
  DEPENDS ${CMAKE_SOURCE_DIR}/shaders/triangle.frag
  COMMENT "glslang: triangle.frag -> SPIR-V")

add_custom_target(shaders DEPENDS ${SHADER_SVPS})

# ---------------------------------------------------------------------------
# Warning policy: -Wall -Wextra everywhere. The Phase 1 exit criteria demand
# zero warnings under this set; treating them globally keeps Phase 1+ code
# honest from day one.
# ---------------------------------------------------------------------------
set(ASTRA_WARN_FLAGS -Wall -Wextra)

# Architecture flags. -mcx16 (CMPXCHG16B) is REQUIRED: the slab allocator's
# ABA-proof free lists use 128-bit double-word CAS. GCC implements 128-bit
# __atomic builtins through the libatomic helper __atomic_compare_exchange_16,
# which runs a cmpxchg16b loop when the target has cx16 (otherwise a correct
# but slower double-lock fallback). Every x86-64 CPU in scope (Ryzen 7 170)
# has cx16; -mcx16 selects the fast path and keeps the code identical on the
# shipping MinGW target. libatomic is part of the GCC toolchain (not an
# external dependency); MinGW-w64 ships it as a static libatomic.a.
set(ASTRA_ARCH_FLAGS -mcx16)
set(ASTRA_ATOMIC_LIB atomic)

# ---------------------------------------------------------------------------
# Shared engine sources (Phase 1: the memory layer)
# ---------------------------------------------------------------------------
set(MEM_SOURCES
    src/memory/slab_allocator.cpp
    src/memory/memory_budget.cpp)

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
# CONSOLE subsystem (no WIN32 flag): the game is launched via .start.bat
# and prints startup banners, the device probe, and the memory report;
# those lines must be visible. A silent GUI-subsystem launcher wrapper is
# a Phase 7 (MP7) polish item.
add_executable(astra src/main.cpp ${MEM_SOURCES})
add_dependencies(astra shaders)
target_include_directories(astra PRIVATE ${VULKAN_HEADERS_DIR} ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(astra PRIVATE VK_NO_PROTOTYPES)
target_compile_options(astra PRIVATE ${ASTRA_WARN_FLAGS} ${ASTRA_ARCH_FLAGS})
# libatomic: the slab allocator's 128-bit CAS (see slab_allocator.h). Part
# of the GCC toolchain; MinGW ships it as a static libatomic.a.
target_link_libraries(astra PRIVATE ${ASTRA_ATOMIC_LIB})
if(WIN32)
  # Static libgcc/libstdc++ so the shipped folder needs no runtime DLLs.
  target_link_options(astra PRIVATE -static-libgcc -static-libstdc++)
else()
  # Sandbox smoke build: dlopen-based loader.
  target_link_libraries(astra PRIVATE ${CMAKE_DL_LIBS})
endif()

set_target_properties(astra PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

# Ship layout: astra.exe with shaders/ alongside.
add_custom_command(TARGET astra POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/output/shaders
  COMMAND ${CMAKE_COMMAND} -E copy ${SHADER_SVPS} ${CMAKE_BINARY_DIR}/output/shaders/
  COMMENT "copying SPIR-V next to the executable")

# ---------------------------------------------------------------------------
# Tests (Phase 1)
# ---------------------------------------------------------------------------
# memory_test: slab allocator + budget tracker (8-thread soak, zero-frag,
# zero-leak, 60 s stress, >10M alloc/s bench).
add_executable(memory_test
    src/memory/memory_test.cpp
    ${MEM_SOURCES})
target_include_directories(memory_test PRIVATE ${VULKAN_HEADERS_DIR} ${CMAKE_SOURCE_DIR}/src)
target_compile_options(memory_test PRIVATE ${ASTRA_WARN_FLAGS} ${ASTRA_ARCH_FLAGS})
target_link_libraries(memory_test PRIVATE ${ASTRA_ATOMIC_LIB})
set_target_properties(memory_test PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

# ring_buffer_test: SPSC ring buffer (1M FIFO, 10M zero-loss MT, >100M
# items/s bench, 60 s stress, stream->gen backpressure demo).
add_executable(ring_buffer_test
    src/core/ring_buffer_test.cpp)
target_include_directories(ring_buffer_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_options(ring_buffer_test PRIVATE ${ASTRA_WARN_FLAGS} ${ASTRA_ARCH_FLAGS})
set_target_properties(ring_buffer_test PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

enable_testing()
add_test(NAME memory_test     COMMAND memory_test)
add_test(NAME ring_buffer_test COMMAND ring_buffer_test)
