cmake_minimum_required(VERSION 3.24)
project(Prism VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "Prism currently targets Windows only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(prism_runtime SHARED
  src/runtime/runtime.cpp
  src/runtime/logger.cpp
  src/runtime/plugin_manager.cpp
)
target_include_directories(prism_runtime PUBLIC include PRIVATE src/runtime)
target_compile_definitions(prism_runtime PRIVATE PRISM_RUNTIME_EXPORTS WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(prism_runtime PRIVATE /W4 /permissive- /EHsc)
set_target_properties(prism_runtime PROPERTIES OUTPUT_NAME "PrismRuntime")

add_executable(prism_launcher WIN32 src/launcher/main.cpp)
target_compile_definitions(prism_launcher PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(prism_launcher PRIVATE /W4 /permissive- /EHsc)
target_link_libraries(prism_launcher PRIVATE comdlg32)
set_target_properties(prism_launcher PROPERTIES OUTPUT_NAME "PrismLauncher")

add_library(prism_sample_mod SHARED examples/hello_mod.cpp)
target_include_directories(prism_sample_mod PRIVATE include)
target_compile_definitions(prism_sample_mod PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(prism_sample_mod PRIVATE /W4 /permissive- /EHsc)
set_target_properties(prism_sample_mod PROPERTIES OUTPUT_NAME "PrismHelloMod")

include(CTest)
if(BUILD_TESTING)
  add_executable(prism_api_tests tests/api_tests.cpp)
  target_include_directories(prism_api_tests PRIVATE include)
  target_compile_options(prism_api_tests PRIVATE /W4 /permissive-)
  add_test(NAME prism_api_tests COMMAND prism_api_tests)
endif()

install(TARGETS prism_runtime prism_launcher prism_sample_mod
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION bin
  ARCHIVE DESTINATION lib
)
install(DIRECTORY include/prism DESTINATION include)
