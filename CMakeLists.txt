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
# Executable
# ---------------------------------------------------------------------------
if(WIN32)
  # WIN32 subsystem (GUI app, no console); MinGW-w64 provides the
  # WinMain -> main() entry shim.
  set(ASTRA_EXE_OPTS WIN32)
else()
  set(ASTRA_EXE_OPTS "")
endif()

add_executable(astra ${ASTRA_EXE_OPTS} src/main.cpp)
add_dependencies(astra shaders)
target_include_directories(astra PRIVATE ${VULKAN_HEADERS_DIR})
target_compile_definitions(astra PRIVATE VK_NO_PROTOTYPES)
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
