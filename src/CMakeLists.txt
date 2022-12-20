cmake_minimum_required(VERSION 3.8)

add_executable(RotateIt "main.cpp" "Core.h" "Core.cpp")

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
set(IMGUI_LIB "${GLOBAL_INCLUDES}/ImGUI")

add_library("ImGUI" STATIC
	"${IMGUI_LIB}/imgui.cpp"	
	"${IMGUI_LIB}/imgui_draw.cpp"	
	"${IMGUI_LIB}/imgui_tables.cpp"	
	"${IMGUI_LIB}/imgui_widgets.cpp"
	"${IMGUI_LIB}/backends/imgui_impl_win32.cpp"
	"${IMGUI_LIB}/backends/imgui_impl_dx11.cpp"
)

add_custom_command(TARGET RotateIt POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Shader/shader.fx ${CMAKE_CURRENT_BINARY_DIR}/shader.fx)

target_link_libraries(RotateIt "${D3D_LIB}")
target_link_libraries(RotateIt "${ASSIMP_LIB}")
target_link_libraries(RotateIt ImGUI)

include_directories("${GLOBAL_INCLUDES}")