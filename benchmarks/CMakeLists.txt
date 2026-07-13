add_executable(sssp_benchmark main.c)

target_compile_options(sssp_benchmark PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-g -O0 -fsanitize=address -fsanitize=undefined>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

target_link_options(sssp_benchmark PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address -fsanitize=undefined>
)

target_link_libraries(sssp_benchmark PRIVATE sssp_algorithms)

set(GRAPH "graphs/test.mtx" CACHE STRING "Path to graph file")
set(SOURCE "0" CACHE STRING "Source vertex index")
set(DELTA "3.0" CACHE STRING "Delta parameter for Delta-Stepping")
set(RUNS "30" CACHE STRING "Number of benchmark iterations")

add_custom_target(run
    COMMAND sssp_benchmark ${GRAPH} ${SOURCE} ${DELTA}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS sssp_benchmark
)

add_custom_target(run-stats
    COMMAND sssp_benchmark ${GRAPH} ${SOURCE} ${DELTA} ${RUNS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS sssp_benchmark
)

add_custom_target(bench
    COMMAND sssp_benchmark ${GRAPH} ${SOURCE} ${DELTA} ${RUNS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS sssp_benchmark
)

add_custom_target(bench-full
    COMMAND sssp_benchmark ${GRAPH} -b ${SOURCE} ${RUNS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS sssp_benchmark
)
