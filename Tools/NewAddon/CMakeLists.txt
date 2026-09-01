cmake_minimum_required(VERSION 3.15)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(NEW_ADDON LANGUAGES CXX C)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    if(MSVC)
        add_compile_options(/utf-8)
    endif()
endif()

add_executable(NewAddon
    src/main.cpp
)

# 考虑到开发者环境大多自备C++运行库，暂不使用MT链接
# if(MSVC)
#     set_property(TARGET NewAddon PROPERTY
#         MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#     set_property(TARGET NewAddon PROPERTY
#         INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

#     target_compile_options(NewAddon PRIVATE
#         $<$<CONFIG:Release>:/O1 /Oi /Gy /Gw /GF /GR- /GS->
#     )

#     target_link_options(NewAddon PRIVATE
#         $<$<CONFIG:Release>:/OPT:REF /OPT:ICF>
#     )
# endif()

target_compile_features(NewAddon PRIVATE cxx_std_20)
