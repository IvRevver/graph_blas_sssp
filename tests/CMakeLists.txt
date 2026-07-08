include_directories(${CMAKE_SOURCE_DIR}/src)

set(SOURCES
    test_all.c
    ${CMAKE_SOURCE_DIR}/src/algorithms/sssp_common.c
    ${CMAKE_SOURCE_DIR}/src/algorithms/lagraph_sssp.c
    ${CMAKE_SOURCE_DIR}/src/algorithms/algebraic_bf_graphblas.c
    ${CMAKE_SOURCE_DIR}/src/algorithms/dijkstra_graphblas.c
    ${CMAKE_SOURCE_DIR}/src/graph/graph_loader.c
    ${CMAKE_SOURCE_DIR}/src/utils/timer.c
    ${CMAKE_SOURCE_DIR}/src/utils/validator.c
)

add_executable(test_sssp ${SOURCES})
target_link_libraries(test_sssp PRIVATE
    GraphBLAS::GraphBLAS
    LAGraph::LAGraph
    OpenMP::OpenMP_C
    m
)

add_test(NAME sssp_tests COMMAND test_sssp WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
