cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("QuModPurge")

if(MSVC)
    add_compile_options(/utf-8)
    add_compile_options(/EHsc)
endif()

add_executable (QuModPurge "QuModPurge.cpp" "EnvEncode.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET QuModPurge PROPERTY CXX_STANDARD 20)
endif()

