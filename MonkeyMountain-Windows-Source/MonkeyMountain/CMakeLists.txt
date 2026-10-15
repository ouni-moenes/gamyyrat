cmake_minimum_required(VERSION 3.20)
project(MonkeyMountain VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest)

option(MONKEY_BUILD_GAME "Build the graphical game (requires raylib)" ON)
if(MONKEY_BUILD_GAME)
    find_package(raylib 5.5 QUIET)
    if(NOT raylib_FOUND)
        include(FetchContent)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG 5.5
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(raylib)
    endif()
    add_executable(MonkeyMountain src/main.cpp)
    target_link_libraries(MonkeyMountain PRIVATE raylib)
    if(WIN32)
        set_target_properties(MonkeyMountain PROPERTIES WIN32_EXECUTABLE TRUE)
        if(MSVC)
            target_link_options(MonkeyMountain PRIVATE /ENTRY:mainCRTStartup)
        endif()
    endif()
    add_custom_command(TARGET MonkeyMountain POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:MonkeyMountain>/assets")
endif()

if(BUILD_TESTING)
    add_executable(game_tests tests/game_tests.cpp)
    add_test(NAME gameplay COMMAND game_tests)
endif()

foreach(target MonkeyMountain game_tests)
    if(TARGET ${target})
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4)
        else()
            target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endif()
endforeach()
