set(SOURCES
    algorithms/sssp_common.c
    algorithms/lagraph_sssp.c
    algorithms/algebraic_bf_graphblas.c
    algorithms/dijkstra_graphblas.c
)

add_library(sssp_algorithms STATIC ${SOURCES})

target_include_directories(sssp_algorithms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(sssp_algorithms PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-g -O0 -fsanitize=address -fsanitize=undefined>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

target_link_options(sssp_algorithms PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address -fsanitize=undefined>
)

target_link_libraries(sssp_algorithms PUBLIC
    GraphBLAS::GraphBLAS
    SuiteSparse::LAGraph
    OpenMP::OpenMP_C
    m
)
