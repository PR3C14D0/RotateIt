cmake_minimum_required(VERSION 3.8)

add_executable(RotateIt "main.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET RotateIt PROPERTY CXX_STANDARD 20)
  set_property(TARGET RotateIt PROPERTY WIN32_EXECUTABLE TRUE)
endif()

add_definitions(-DUNICODE)
add_compile_definitions(NOMINMAX)

set(GLOBAL_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../include")
set(LIB "${CMAKE_CURRENT_SOURCE_DIR}/../lib")
set(D3D_LIB "${LIB}/DirectX/D3D11.lib;${LIB}/DirectX/D3DX11.lib;${LIB}/DirectX/DXGI.lib")
set(ASSIMP_LIB "${LIB}/Assimp/assimp.lib")

target_link_libraries(RotateIt "${D3D_LIB}")
target_link_libraries(RotateIt "${ASSIMP_LIB}")

include_directories("${GLOBAL_INCLUDES}")