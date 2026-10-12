cmake_minimum_required(VERSION 3.10)
project(ExtreamFS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

# Buscar librerías
find_package(Threads REQUIRED)

# Incluir directorios
include_directories(include)
include_directories(.)
include_directories(sistema)
include_directories(lexer)
include_directories(parser)

# Archivos fuente (buscar TODOS los .cpp recursivamente)
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "sistema/*.cpp"
    "lexer/*.cpp"
    "parser/*.cpp"
    "main.cpp"
)

# Crear ejecutable
add_executable(extreamfs ${SOURCES})

# Enlazar librerías
target_link_libraries(extreamfs 
    Threads::Threads
    pthread
)