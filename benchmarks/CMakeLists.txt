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
