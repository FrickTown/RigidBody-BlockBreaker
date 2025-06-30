#if (EMSCRIPTEN)
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1")
#    set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your executable to build with the emscripten html template so that you can directly open it.
#endif ()
cmake_minimum_required(VERSION 3.22)
set(CMAKE_C_STANDARD 23)
include(FetchContent)

project(Box2DTest
VERSION 1.0.0
DESCRIPTION "Shows how to integrate Box2D with raylib"
HOMEPAGE_URL "https://box2d.org"
LANGUAGES C
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#FetchContent_Declare(
#box2d
#GIT_REPOSITORY https://github.com/erincatto/box2d.git
#GIT_TAG main
#GIT_SHALLOW TRUE
#GIT_PROGRESS TRUE
#)
#FetchContent_MakeAvailable(box2d)


#FetchContent_Declare(
#raylib
#GIT_REPOSITORY https://github.com/raysan5/raylib.git
#GIT_TAG master
#GIT_SHALLOW TRUE
#GIT_PROGRESS TRUE
#)
#FetchContent_MakeAvailable(raylib)

include_directories("/home/frick/Dev/C-Stuff/raylib-5.5_webassembly/include" "/home/frick/Dev/C-Stuff/emsdk/upstream/emscripten/cache/sysroot/include" "/home/frick/Dev/C-Stuff/box2d-3.1.1/include" "/home/frick/Dev/C-Stuff/box2d-3.1.1/src/")
link_directories("/home/frick/Dev/C-Stuff/raylib-5.5_webassembly/lib" "/home/frick/Dev/C-Stuff/emsdk/upstream/emscripten/cache/sysroot/lib" "/home/frick/Dev/C-Stuff/box2d-3.1.1/build/src/")

#configure_file(assets/paddle.ogg assets/paddle.ogg COPYONLY)
file(COPY assets DESTINATION .)

add_executable(Box2DTest main.c
        entities.c
        entities.h)
target_link_libraries(Box2DTest PRIVATE box2d raylib m)

if (MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Box2DTest)
    set_property(TARGET Box2DTest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

if (EMSCRIPTEN)
#emcc -o build_web/game.html main.c entities.c --preload-file assets -std=c23 -Os -Wall /home/frick/Dev/C-Stuff/raylib-5.5_webassembly/lib/libraylib.a -I. -I/home/frick/Dev/C-Stuff/box2d-3.1.1/src/ -I/home/frick/Dev/C-Stuff/box2d-3.1.1/build/src/ -I/home/frick/Dev/C-Stuff/box2d-3.1.1/include/ -I/home/frick/Dev/C-Stuff/raylib-5.5_webassembly/include/ /home/frick/Dev/C-Stuff/box2d-3.1.1/build/src/CMakeFiles/box2d.dir/*.o -L. -L/home/frick/Dev/C-Stuff/raylib-5.5_webassembly/lib/libraylib.a -L/home/frick/Dev/C-Stuff/box2d-3.1.1/build/src/libbox2dd.a -s USE_GLFW=3 --shell-file ./html_template/shell_minimal.html -DPLATFORM_WEB -lembind
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
    # Since WASM is used, ALLOW_MEMORY_GROWTH has no extra overheads
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm -sWASM=1 -sASYNCIFY -sALLOW_MEMORY_GROWTH=1 --shell-file ${CMAKE_SOURCE_DIR}/minshell.html")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    #target_link_libraries(Box2DTest INTERFACE "--preload-file images@images -sWASM=1 -sASYNCIFY -sALLOW_MEMORY_GROWTH=1 --shell-file ${CMAKE_SOURCE_DIR}/minshell.html")
endif()