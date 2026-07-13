set(SOURCES
    main.c
    algorithms/sssp_common.c
    algorithms/lagraph_sssp.c
    algorithms/algebraic_bf_graphblas.c
    algorithms/dijkstra_graphblas.c
    graph/graph_loader.c
    utils/timer.c
    utils/validator.c
)

add_executable(sssp_benchmark ${SOURCES})

target_include_directories(sssp_benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(sssp_benchmark PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-g -O0 -fsanitize=address -fsanitize=undefined>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

target_link_options(sssp_benchmark PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address -fsanitize=undefined>
)

target_link_libraries(sssp_benchmark PRIVATE
    GraphBLAS::GraphBLAS
    SuiteSparse::LAGraph
    OpenMP::OpenMP_C
    m
)
