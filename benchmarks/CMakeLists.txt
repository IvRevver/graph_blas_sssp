set(GRAPH "graphs/test.mtx" CACHE STRING "Path to graph file")
set(SOURCE "0" CACHE STRING "Source vertex index")
set(DELTA "3.0" CACHE STRING "Delta parameter for Delta-Stepping")
set(RUNS "30" CACHE STRING "Number of benchmark iterations")
set(DELTAS "0.5 1.0 2.0 3.0 5.0 10.0" CACHE STRING "Delta values for sweep")

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/results)

add_custom_target(run
    COMMAND sssp_benchmark ${GRAPH} ${SOURCE} ${DELTA}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS sssp_benchmark
    COMMENT "Running benchmark"
)

add_custom_target(run-graph
    COMMAND sssp_benchmark ${GRAPH} ${SOURCE} ${DELTA}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS sssp_benchmark
)

add_custom_target(run-stats
    COMMAND sssp_benchmark ${GRAPH} ${SOURCE} ${DELTA} ${RUNS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS sssp_benchmark
)

add_custom_target(run-delta-sweep
    COMMAND bash -c "i=0; for d in ${DELTAS}; do i=\$((i+1)); if [ \$i -eq 1 ]; then ${CMAKE_SOURCE_DIR}/bin/sssp_benchmark '${GRAPH}' '${SOURCE}' \$d 2>&1 | awk -v d=\"\$d\" '/\\[1\\/3\\]/ { a = sprintf(\"DS delta=%s\", d); next } /\\[2\\/3\\]/ { a = \"Algebraic BF\"; next } /\\[3\\/3\\]/ { a = \"Dijkstra\"; next } a != \"\" && /^  \\[OK\\] [0-9]/ { gsub(/,/,\"\",\$5); gsub(/,/,\"\",\$7); printf \"| %-22s | %7.2f | %5d | %9d | [OK]  |\\n\", a, \$2, \$5, \$7; a = \"\" }'; else ${CMAKE_SOURCE_DIR}/bin/sssp_benchmark '${GRAPH}' '${SOURCE}' \$d 2>&1 | awk -v d=\"\$d\" '/\\[1\\/3\\]/ { a = sprintf(\"DS delta=%s\", d); next } a != \"\" && /^  \\[OK\\] [0-9]/ { gsub(/,/,\"\",\$5); gsub(/,/,\"\",\$7); printf \"| %-22s | %7.2f | %5d | %9d | [OK]  |\\n\", a, \$2, \$5, \$7; a = \"\" }'; fi; done"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS sssp_benchmark
    COMMENT "Running delta sweep"
)

add_custom_target(run-delta-stats
    COMMAND bash -c "first=1; for d in ${DELTAS}; do if [ \$first -eq 1 ]; then ${CMAKE_SOURCE_DIR}/bin/sssp_benchmark '${GRAPH}' '${SOURCE}' \$d ${RUNS} 2>&1 | awk -F'|' -v d=\"\$d\" '/Algebraic.*\\(GB\\)/ { abf = sprintf(\"Algebraic BF              %7.2f %7.2f %7.2f %7.2f\\n\", \$3+0, \$4+0, \$5+0, \$6+0); next } /Dijkstra.*\\(GB\\)/ { dij = sprintf(\"Dijkstra                  %7.2f %7.2f %7.2f %7.2f\\n\", \$3+0, \$4+0, \$5+0, \$6+0); next } /Delta-Stepping.*\\(LAG\\)/ { ds = sprintf(\"DS delta=%-5s            %7.2f %7.2f %7.2f %7.2f\\n\", d, \$3+0, \$4+0, \$5+0, \$6+0) } END { printf \"%s%s%s\", abf, dij, ds }'; first=0; else ${CMAKE_SOURCE_DIR}/bin/sssp_benchmark '${GRAPH}' '${SOURCE}' \$d ${RUNS} 2>&1 | awk -F'|' -v d=\"\$d\" '/Delta-Stepping.*\\(LAG\\)/ { printf \"DS delta=%-5s            %7.2f %7.2f %7.2f %7.2f\\n\", d, \$3+0, \$4+0, \$5+0, \$6+0 }'; fi; done"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS sssp_benchmark
    COMMENT "Running delta stats sweep"
)
