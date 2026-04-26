cmake_minimum_required(VERSION 3.18)
project(as6 CXX)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(../raylib-cpp raylib)

include(../assets/includeable.cmake)

add_executable(as6 as6.cpp)
target_link_libraries(as6 raylib raylib_cpp raygui raylib::buffered)

configure_file(../assets/models/nj.glb models/nj.glb COPYONLY)
configure_file(../assets/models/pe.glb models/pe.glb COPYONLY)
configure_file(../assets/audio/New_Jersey_SkillActivationJP.wav audio/New_Jersey_SkillActivationJP.wav COPYONLY)
configure_file(../assets/audio/Prinz_Eugen_SkillActivationJP.wav audio/Prinz_Eugen_SkillActivationJP.wav COPYONLY)