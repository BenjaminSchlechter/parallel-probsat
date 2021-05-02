cmake_minimum_required(VERSION 3.4.3)

project(PROBSAT)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "Release")

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g -Wall -Wextra")
# SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -Wextra")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -Wextra -funroll-loops -fexpensive-optimizations")

find_package(MPI REQUIRED)
find_package(Threads REQUIRED)

include_directories(${MPI_CXX_INCLUDE_PATH})
link_directories(${MPI_CXX_LINK_FLAG})

set(BITSERY_DIRECTORY "./bitsery/include/")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

add_executable (manager manager.cpp)
target_include_directories(manager PRIVATE ${BITSERY_DIRECTORY})
target_link_libraries(manager ${MPI_LIBRARIES})

add_executable (server server.cpp)
target_include_directories(server PRIVATE ${BITSERY_DIRECTORY})
target_link_libraries(server ${MPI_LIBRARIES})

add_executable (worker worker.cpp)
target_include_directories(worker PRIVATE ${BITSERY_DIRECTORY})
target_link_libraries(worker ${MPI_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable (probsat probsat.cpp)
