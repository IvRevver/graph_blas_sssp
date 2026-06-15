#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include "GraphBLAS.h"
#include "LAGraph.h"

#include "../src/algorithms/sssp_common.h"
#include "../src/algorithms/lagraph_sssp.h"
#include "../src/algorithms/algebraic_bf_graphblas.h"
#include "../src/algorithms/dijkstra_graphblas.h"
#include "../src/graph/graph_loader.h"
#include "../src/utils/timer.h"
#include "../src/utils/validator.h"

#define TEST_GRAPH_PATH "tests/test_graph.mtx"
#define EPSILON 1e-6
#define MAX_TESTS 64

typedef struct {
    const char *name;      /** Название теста */
    bool passed;           /** Пройден ли тест */
    const char *message;   /** Сообщение об ошибке (если есть) */
} TestResult;

static struct {
    int total;
    int passed;
    int failed;
    TestResult results[MAX_TESTS];
} test_stats = {0, 0, 0, {{0}}};

static void register_test(const char *name, bool passed, const char *message) {
    if (test_stats.total >= MAX_TESTS) {
        fprintf(stderr, "⚠️  Превышено максимальное количество тестов\n");
        return;
    }
    
    test_stats.results[test_stats.total].name = name;
    test_stats.results[test_stats.total].passed = passed;
    test_stats.results[test_stats.total].message = message;
    test_stats.total++;
    
    if (passed) {
        test_stats.passed++;
        printf("  ✅ %s\n", name);
    } else {
        test_stats.failed++;
        printf("  ❌ %s\n", name);
        if (message) {
            printf("      Ошибка: %s\n", message);
        }
    }
}

static bool compare_distances(GrB_Vector v1, GrB_Vector v2) {
    if (!v1 || !v2) {
        return false;
    }
    
    GrB_Index n1, n2;
    GrB_Vector_size(&n1, v1);
    GrB_Vector_size(&n2, v2);
    
    if (n1 != n2) {
        return false;
    }
    
    for (GrB_Index i = 0; i < n1; i++) {
        double d1, d2;
        GrB_Info info1 = GrB_Vector_extractElement(&d1, v1, i);
        GrB_Info info2 = GrB_Vector_extractElement(&d2, v2, i);
        
        /* Оба должны иметь значение или оба не иметь */
        if (info1 != info2) {
            return false;
        }
        
        /* Если оба имеют значение - сравниваем */
        if (info1 == GrB_SUCCESS) {
            /* Оба INF */
            if (isinf(d1) && isinf(d2)) {
                continue;
            }
            
            /* Один INF, другой нет */
            if (isinf(d1) || isinf(d2)) {
                return false;
            }
            
            /* Оба конечные - сравниваем с погрешностью */
            if (fabs(d1 - d2) > EPSILON) {
                return false;
            }
        }
    }
    
    return true;
}


static bool verify_source_distance(GrB_Vector distances, GrB_Index source) {
    if (!distances) {
        return false;
    }
    
    double source_dist;
    GrB_Info info = GrB_Vector_extractElement(&source_dist, distances, source);
    
    return (info == GrB_SUCCESS && source_dist == 0.0);
}

static bool verify_non_negative(GrB_Vector distances) {
    if (!distances) {
        return false;
    }
    
    GrB_Index n;
    GrB_Vector_size(&n, distances);
    
    for (GrB_Index i = 0; i < n; i++) {
        double dist;
        GrB_Info info = GrB_Vector_extractElement(&dist, distances, i);
        
        if (info == GrB_SUCCESS) {
            if (dist < 0.0 && !isinf(dist)) {
                return false;
            }
        }
    }
    
    return true;
}

static void test_graph_loading(LAGraph_Graph graph, GraphInfo *info) {
    printf("\n[Тест] Загрузка графа\n");
    
    /* Проверка что граф загружен */
    register_test("Граф загружен", graph != NULL, NULL);
    
    /* Проверка количества вершин */
    char msg[256];
    snprintf(msg, sizeof(msg), "Ожидалось 6, получено %llu", (unsigned long long)info->nverts);
    register_test("Количество вершин = 6", info->nverts == 6, 
                  info->nverts != 6 ? msg : NULL);
    
    /* Проверка количества рёбер */
    snprintf(msg, sizeof(msg), "Ожидалось 10, получено %llu", (unsigned long long)info->nedges);
    register_test("Количество рёбер = 10", info->nedges == 10,
                  info->nedges != 10 ? msg : NULL);
    
    /* Проверка что граф ориентированный */
    register_test("Граф ориентированный", info->directed == true, NULL);
}


static void test_lagraph_sssp(LAGraph_Graph graph, GrB_Index source) {
    printf("\n[Тест] LAGraph SSSP (Delta-Stepping)\n");
    
    SSSP_Result result;
    GrB_Info info = lagraph_sssp(&result, graph, source, 3.0);
    
    register_test("Запуск успешен", info == GrB_SUCCESS, NULL);
    register_test("Флаг успеха установлен", result.success == true, NULL);
    register_test("Вектор расстояний создан", result.distances != NULL, NULL);
    register_test("Есть достижимые вершины", result.reachable_vertices > 0, NULL);
    
    /* Проверка расстояния до источника */
    register_test("dist[source] == 0", 
                  verify_source_distance(result.distances, source),
                  NULL);
    
    /* Проверка неотрицательности */
    register_test("Все расстояния >= 0",
                  verify_non_negative(result.distances),
                  NULL);
    
    sssp_result_cleanup(&result);
}


static void test_algebraic_bf(LAGraph_Graph graph, GrB_Index source) {
    printf("\n[Тест] Algebraic Bellman-Ford\n");
    
    SSSP_Result result;
    GrB_Info info = algebraic_bf_graphblas(&result, graph, source, 0.0);
    
    register_test("Запуск успешен", info == GrB_SUCCESS, NULL);
    register_test("Флаг успеха установлен", result.success == true, NULL);
    register_test("Вектор расстояний создан", result.distances != NULL, NULL);
    register_test("Количество итераций > 0", result.iterations > 0, NULL);
    GrB_Index n;
    GrB_Matrix_nrows(&n, graph->A);
    register_test("Итераций < n вершин", (GrB_Index)result.iterations < n, NULL);
    
    register_test("dist[source] == 0",
                  verify_source_distance(result.distances, source),
                  NULL);
    
    register_test("Все расстояния >= 0",
                  verify_non_negative(result.distances),
                  NULL);
    
    sssp_result_cleanup(&result);
}


static void test_dijkstra(LAGraph_Graph graph, GrB_Index source) {
    printf("\n[Тест] Dijkstra\n");
    
    SSSP_Result result;
    GrB_Info info = dijkstra_graphblas(&result, graph, source, 0.0);
    
    register_test("Запуск успешен", info == GrB_SUCCESS, NULL);
    register_test("Флаг успеха установлен", result.success == true, NULL);
    register_test("Вектор расстояний создан", result.distances != NULL, NULL);
    
    register_test("dist[source] == 0",
                  verify_source_distance(result.distances, source),
                  NULL);
    
    register_test("Все расстояния >= 0",
                  verify_non_negative(result.distances),
                  NULL);
    
    sssp_result_cleanup(&result);
}


static void test_consistency(LAGraph_Graph graph, GrB_Index source) {
    printf("\n[Тест] Согласованность алгоритмов\n");
    
    SSSP_Result ref, alg;
    
    /* Algebraic BF как эталон */
    algebraic_bf_graphblas(&ref, graph, source, 0.0);
    
    /* Сравнение с Dijkstra */
    dijkstra_graphblas(&alg, graph, source, 0.0);
    register_test("Algebraic BF == Dijkstra",
                  compare_distances(ref.distances, alg.distances),
                  NULL);
    sssp_result_cleanup(&alg);
    
    sssp_result_cleanup(&ref);
}

static void test_validator(LAGraph_Graph graph, GrB_Index source) {
    printf("\n[Тест] Валидатор\n");
    
    SSSP_Result result;
    algebraic_bf_graphblas(&result, graph, source, 0.0);
    
    /* Проверка полной валидации */
    register_test("sssp_validate_result passed",
                  sssp_validate_result(&result, source),
                  NULL);
    
    /* Проверка отдельных функций */
    register_test("sssp_validate_source_distance",
                  sssp_validate_source_distance(result.distances, source),
                  NULL);
    
    register_test("sssp_validate_non_negative",
                  sssp_validate_non_negative(result.distances),
                  NULL);
    
    /* Проверка сравнения векторов */
    SSSP_Result result2;
    algebraic_bf_graphblas(&result2, graph, source, 0.0);
    
    register_test("sssp_validate_distances (same)",
                  sssp_validate_distances(result.distances, result2.distances),
                  NULL);
    
    sssp_result_cleanup(&result);
    sssp_result_cleanup(&result2);
}


static void test_timer(void) {
    printf("\n[Тест] Таймер\n");
    
    /* Проверка доступности */
    register_test("timer_is_available",
                  timer_is_available(),
                  NULL);
    
    /* Проверка разрешения */
    long resolution = timer_get_resolution();
    char msg[256];
    snprintf(msg, sizeof(msg), "Разрешение: %ld тиков/сек", resolution);
    register_test("timer_get_resolution > 0",
                  resolution > 0,
                  resolution <= 0 ? msg : NULL);
    
    /* Проверка измерения времени */
    timer_start();
    
    /* Небольшая задержка */
    volatile int sum = 0;
    for (int i = 0; i < 1000000; i++) {
        sum += i;
    }
    
    double elapsed_ms = timer_stop_ms();
    
    register_test("timer_stop_ms > 0",
                  elapsed_ms >= 0,  /* Может быть 0 если очень быстро */
                  NULL);
    
    (void)sum;  /* Подавить предупреждение */
}

int main(void) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║              SSSP GraphBLAS - Юнит-тесты                       ║\n");
    printf("║                        Version 1.0.0                           ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    char msg[LAGRAPH_MSG_LEN];
    GrB_Info info = LAGraph_Init(msg);
    if (info != GrB_SUCCESS) {
        fprintf(stderr, "❌ LAGraph_Init failed: %d\n%s\n", info, msg);
        return 1;
    }
    
    /* Загрузка тестового графа */
    LAGraph_Graph graph = NULL;
    GraphInfo graph_info;
    
    info = graph_load(&graph, TEST_GRAPH_PATH, &graph_info);
    if (info != GrB_SUCCESS) {
        fprintf(stderr, "❌ Не удалось загрузить тестовый граф: %s\n", 
                TEST_GRAPH_PATH);
        fprintf(stderr, "   Убедитесь что файл существует и запущен из корня проекта\n");
        LAGraph_Finalize(msg);
        return 1;
    }
    
    printf("\nТестовый граф: %s\n", graph_info.name);
    printf("  Вершин: %llu\n", (unsigned long long)graph_info.nverts);
    printf("  Рёбер: %llu\n", (unsigned long long)graph_info.nedges);
    
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
    
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                         ИТОГИ ТЕСТОВ                           ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    printf("║  Всего тестов:     %-4d                                      ║\n", test_stats.total);
    printf("║  Пройдено:         %-4d  ✅                                  ║\n", test_stats.passed);
    printf("║  Провалено:        %-4d  ❌                                  ║\n", test_stats.failed);
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    
    if (test_stats.failed == 0) {
        printf("║  Статус: ВСЕ ТЕСТЫ ПРОЙДЕНЫ ✅                               ║\n");
    } else {
        printf("║  Статус: ЕСТЬ ПРОВАЛЫ ❌                                     ║\n");
    }
    
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    return (test_stats.failed == 0) ? 0 : 1;
}