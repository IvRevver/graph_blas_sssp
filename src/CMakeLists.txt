include_directories(${CMAKE_CURRENT_SOURCE_DIR})

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
target_link_libraries(sssp_benchmark PRIVATE
    GraphBLAS::GraphBLAS
    SuiteSparse::LAGraph
    OpenMP::OpenMP_C
    m
)
