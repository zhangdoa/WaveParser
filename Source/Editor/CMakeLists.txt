add_subdirectory("ImGuiWrapper")
file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_executable(WsEditor ${HEADERS} ${SOURCES})
target_link_libraries(WsEditor ImGuiWrapper)
target_link_libraries(WsEditor WsRuntime)
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set_target_properties(WsEditor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()