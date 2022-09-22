cmake_minimum_required(VERSION 3.16)
project(abwd_benchmark)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

add_executable(${PROJECT_NAME} src/main.cpp src/taos_client.cpp src/latch.cpp src/statement.cpp include/statement_data.h src/statement_data.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt pthread taos)