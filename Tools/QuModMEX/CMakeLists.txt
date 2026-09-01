cmake_minimum_required (VERSION 3.15)

if (POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("QuModMEX")

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    if(MSVC)
        add_compile_options(/utf-8 /EHsc)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:>")
    endif()
endif()

set(UTIL_SOURCE "utils/EnvEncode.cpp")

add_executable(QuModMEX 
	"main.cpp"
	"src/QuModMEX.cpp"
	${UTIL_SOURCE}
)

target_compile_features(QuModMEX PRIVATE cxx_std_20)

target_include_directories(QuModMEX PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)