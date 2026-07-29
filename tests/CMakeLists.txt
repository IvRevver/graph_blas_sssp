add_executable(test_sssp test_all.c
    ${CMAKE_SOURCE_DIR}/benchmarks/graph/graph_loader.c
    ${CMAKE_SOURCE_DIR}/benchmarks/utils/timer.c
    ${CMAKE_SOURCE_DIR}/benchmarks/utils/validator.c
)

target_include_directories(test_sssp PRIVATE ${CMAKE_SOURCE_DIR}/benchmarks)

target_compile_options(test_sssp PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-g -O0 -fsanitize=address -fsanitize=undefined>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

target_link_options(test_sssp PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address -fsanitize=undefined>
)

target_link_libraries(test_sssp PRIVATE sssp_algorithms)

add_test(NAME sssp_tests COMMAND test_sssp WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
