# cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
# cmake --build build

cmake_minimum_required(VERSION 3.10)

project(nano8 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)
find_package(cJSON REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LUA REQUIRED lua)

# include directories
include_directories(
  ${PROJECT_SOURCE_DIR}/includes
)

set(SOURCES
  src/main.c 
  src/scene.c 
  src/font.c 
  src/game.c 
  src/console.c 
  src/luaapi.c 
  src/sprite.c
  src/editor.c
  src/maps.c 
  src/sfx.c
  src/cartridge.c
  src/nanoUI.c
)

# Add executable using main.c and the generated lexer.c
add_executable(nano8  ${SOURCES})

target_include_directories(nano8 PRIVATE ${LUA_INCLUDE_DIRS})

target_compile_options(nano8
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -fsanitize=address
        -fno-omit-frame-pointer
)

target_link_options(nano8
    PRIVATE
        -fsanitize=address
        -lm
)

target_link_libraries(nano8
  PRIVATE raylib cjson ${LUA_LIBRARIES}
)

# Web-specific flags
if(EMSCRIPTEN)
  set_target_properties(nano8 PROPERTIES SUFFIX ".html" OUTPUT_NAME "index")

    target_link_options(nano8 PRIVATE
        "--shell-file" "${CMAKE_SOURCE_DIR}/resources/html/shell.html"
        "-sUSE_GLFW=3"
        "-sASYNCIFY"
        "-sFORCE_FILESYSTEM=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sENVIRONMENT=web"
        "-sGL_ENABLE_GET_PROC_ADDRESS=1"
        "--preload-file" "${CMAKE_SOURCE_DIR}/resources@/resources"
    )
endif()
