#include <stdio.h>
#include <stdbool.h>

#include "GraphBLAS.h"
#include "LAGraph.h"

#include "algorithms/sssp_common.h"
#include "algorithms/lagraph_sssp.h"
#include "algorithms/algebraic_bf_graphblas.h"
#include "algorithms/dijkstra_graphblas.h"
#include "graph/graph_loader.h"
#include "utils/timer.h"
#include "utils/validator.h"

#define TEST_GRAPH_PATH "tests/test_graph.mtx"
#define MAX_TESTS 64

typedef struct {
    const char *name;    /** Название теста */
    bool passed;         /** Пройден ли тест */
    const char *message; /** Сообщение об ошибке (если есть) */
} TestResult;

static struct {
    int total;
    int passed;
    int failed;
    TestResult results[MAX_TESTS];
} test_stats = {0, 0, 0, {{0}}};

static void register_test(const char *name, bool passed, const char *message) {
    if (test_stats.total >= MAX_TESTS) {
        fprintf(stderr, "[!] Max test count exceeded\n");
        return;
    }

    test_stats.results[test_stats.total].name = name;
    test_stats.results[test_stats.total].passed = passed;
    test_stats.results[test_stats.total].message = message;
    test_stats.total++;

    if (passed) {
        test_stats.passed++;
        printf("  [OK] %s\n", name);
    } else {
        test_stats.failed++;
        printf("  [FAIL] %s\n", name);
        if (message) {
            printf("      Ошибка: %s\n", message);
        }
    }
}

static void test_graph_loading(LAGraph_Graph graph, GraphInfo *info) {
    printf("\n[Test] Graph loading\n");

    /* Проверка что граф загружен */
    register_test("Graph loaded", graph != NULL, NULL);

    /* Проверка количества вершин */
    char msg[256];
    snprintf(msg, sizeof(msg), "Expected 6, got %llu", (unsigned long long)info->nverts);
    register_test("Vertices = 6", info->nverts == 6, info->nverts != 6 ? msg : NULL);

    /* Проверка количества рёбер */
    snprintf(msg, sizeof(msg), "Expected 10, got %llu", (unsigned long long)info->nedges);
    register_test("Edges = 10", info->nedges == 10, info->nedges != 10 ? msg : NULL);

    /* Проверка что граф ориентированный */
    register_test("Directed", info->directed == true, NULL);
}


static void test_lagraph_sssp(LAGraph_Graph graph, GrB_Index source) {
    printf("\n[Test] LAGraph SSSP (Delta-Stepping)\n");

    SSSP_Result result;
    GrB_Info info = lagraph_sssp(&result, graph, source, 3.0);

    register_test("Init OK", info == GrB_SUCCESS, NULL);
    register_test("Success flag", result.success == true, NULL);
    register_test("Distances vector", result.distances != NULL, NULL);
    register_test("Has reachable vertices", result.reachable_vertices > 0, NULL);

    /* Проверка расстояния до источника */
    register_test("dist[source] == 0", sssp_validate_source_distance(result.distances, source),
                  NULL);

    register_test("All distances >= 0", sssp_validate_non_negative(result.distances), NULL);

    sssp_result_cleanup(&result);
}


static void test_algebraic_bf(LAGraph_Graph graph, GrB_Index source) {
    printf("\n[Test] Algebraic Bellman-Ford\n");

    SSSP_Result result;
    GrB_Info info = algebraic_bf_graphblas(&result, graph, source, 0.0);

    register_test("Init OK", info == GrB_SUCCESS, NULL);
    register_test("Success flag", result.success == true, NULL);
    register_test("Distances vector", result.distances != NULL, NULL);
    register_test("K > 0", result.iterations > 0, NULL);
    GrB_Index n;
    GrB_Matrix_nrows(&n, graph->A);
    register_test("Iterations < n", (GrB_Index)result.iterations < n, NULL);

    register_test("dist[source] == 0", sssp_validate_source_distance(result.distances, source),
                  NULL);

    register_test("All distances >= 0", sssp_validate_non_negative(result.distances), NULL);

    sssp_result_cleanup(&result);
}


static void test_dijkstra(LAGraph_Graph graph, GrB_Index source) {
    printf("\n[Test] Dijkstra\n");

    SSSP_Result result;
    GrB_Info info = dijkstra_graphblas(&result, graph, source, 0.0);

    register_test("Init OK", info == GrB_SUCCESS, NULL);
    register_test("Success flag", result.success == true, NULL);
    register_test("Distances vector", result.distances != NULL, NULL);

    register_test("dist[source] == 0", sssp_validate_source_distance(result.distances, source),
                  NULL);

    register_test("All distances >= 0", sssp_validate_non_negative(result.distances), NULL);

    sssp_result_cleanup(&result);
}


static void test_consistency(LAGraph_Graph graph, GrB_Index source) {
    printf("\n[Test] Algorithm consistency\n");

    SSSP_Result ref, alg;

    /* Algebraic BF как эталон */
    algebraic_bf_graphblas(&ref, graph, source, 0.0);

    /* Сравнение с Dijkstra */
    dijkstra_graphblas(&alg, graph, source, 0.0);
    register_test("Algebraic BF == Dijkstra", sssp_validate_distances(ref.distances, alg.distances),
                  NULL);
    sssp_result_cleanup(&alg);

    sssp_result_cleanup(&ref);
}

static void test_validator(LAGraph_Graph graph, GrB_Index source) {
    printf("\n[Test] Validator\n");

    SSSP_Result result;
    algebraic_bf_graphblas(&result, graph, source, 0.0);

    /* Проверка полной валидации */
    register_test("sssp_validate_result passed", sssp_validate_result(&result, source), NULL);

    /* Проверка отдельных функций */
    register_test("sssp_validate_source_distance",
                  sssp_validate_source_distance(result.distances, source), NULL);

    register_test("sssp_validate_non_negative", sssp_validate_non_negative(result.distances), NULL);

    /* Проверка сравнения векторов */
    SSSP_Result result2;
    algebraic_bf_graphblas(&result2, graph, source, 0.0);

    register_test("sssp_validate_distances (same)",
                  sssp_validate_distances(result.distances, result2.distances), NULL);

    sssp_result_cleanup(&result);
    sssp_result_cleanup(&result2);
}


static void test_timer(void) {
    printf("\n[Test] Timer\n");

    timer_start();

    volatile unsigned sum = 0;
    for (unsigned i = 0; i < 1000000; i++) {
        sum += i;
    }

    double elapsed_ms = timer_stop_ms();

    register_test("timer_stop_ms >= 0", elapsed_ms >= 0, /* Может быть 0 если очень быстро */
                  NULL);

    (void)sum; /* Подавить предупреждение */
}

int main(void) {
    printf("+--------------------------------------------------------------+\n");
    printf("| SSSP GraphBLAS - Unit Tests                                  |\n");
    printf("| Version 1.0.0                                                |\n");
    printf("+--------------------------------------------------------------+\n");

    char msg[LAGRAPH_MSG_LEN];
    GrB_Info info = LAGraph_Init(msg);
    if (info != GrB_SUCCESS) {
        fprintf(stderr, "[FAIL] LAGraph_Init failed: %d\n%s\n", info, msg);
        return 1;
    }

    /* Disable JIT: LAGraph SSSP triggers GrB_select/assign kernels that
       fail with GxB_JIT_ERROR (-7001) on CI. Pre-compiled kernels suffice. */
    GxB_Global_Option_set_INT32(GxB_JIT_C_CONTROL, GxB_JIT_OFF);

    /* Загрузка тестового графа */
    LAGraph_Graph graph = NULL;
    GraphInfo graph_info;

    info = graph_load(&graph, TEST_GRAPH_PATH, &graph_info);
    if (info != GrB_SUCCESS) {
        fprintf(stderr, "[FAIL] Could not load test graph: %s\n", TEST_GRAPH_PATH);
        fprintf(stderr, "       Ensure the file exists and run from project root\n");
        LAGraph_Finalize(msg);
        return 1;
    }

    LAGraph_Cached_EMin(graph, msg);

    printf("\nTest graph: %s\n", graph_info.name);
    printf("  Vertices: %llu\n", (unsigned long long)graph_info.nverts);
    printf("  Edges: %llu\n", (unsigned long long)graph_info.nedges);

    GrB_Index source = 0;

    /* Тесты загрузки графа */
    test_graph_loading(graph, &graph_info);

    /* Тесты алгоритмов */
    test_lagraph_sssp(graph, source);
    test_algebraic_bf(graph, source);
    test_dijkstra(graph, source);

    /* Тесты согласованности */
    test_consistency(graph, source);

    /* Тесты утилит */
    test_validator(graph, source);
    test_timer();

    LAGraph_Delete(&graph, msg);
    LAGraph_Finalize(msg);

    printf("\n+--------------------------------------------------------------+\n");
    printf("|                         TEST RESULTS                          |\n");
    printf("+--------------------------------------------------------------+\n");
    {
        char line[64];
        snprintf(line, sizeof(line), "Total: %d", test_stats.total);
        printf("| %-60s |\n", line);
        snprintf(line, sizeof(line), "Passed: %d", test_stats.passed);
        printf("| %-60s |\n", line);
        snprintf(line, sizeof(line), "Failed: %d", test_stats.failed);
        printf("| %-60s |\n", line);
    }
    printf("+--------------------------------------------------------------+\n");

    if (test_stats.failed == 0) {
        printf("| %-60s |\n", "Status: ALL TESTS PASSED");
    } else {
        printf("| %-60s |\n", "Status: SOME TESTS FAILED");
    }

    printf("+--------------------------------------------------------------+\n");

    return (test_stats.failed == 0) ? 0 : 1;
}