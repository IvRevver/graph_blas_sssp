#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

#define VERSION "1.0.0"

#define MAX_ALGORITHMS 3

#define DISTANCE_EPSILON 1e-6


static void print_usage(const char *prog) {
    printf("Использование: %s <файл.mtx> [source] [delta]\n", prog);
    printf("\n");
    printf("Аргументы:\n");
    printf("  <файл.mtx>    Путь к графу в формате Matrix Market [3]\n");
    printf("  source        Исходная вершина (по умолчанию: 0)\n");
    printf("  delta         Параметр для Delta-Stepping (по умолчанию: 3.0)\n");
    printf("\n");
    printf("Примеры:\n");
    printf("  %s graphs/test.mtx\n", prog);
    printf("  %s graphs/test.mtx 0 3.0\n", prog);
    printf("  %s graphs/road-net-CA.mtx 5 5.0\n", prog);
    printf("\n");
    printf("Алгоритмы:\n");
    printf("  1. Delta-Stepping (LAGr_SingleSourceShortestPath)\n");
    printf("  2. Algebraic Bellman-Ford (Raw GraphBLAS)\n");
    printf("  3. Dijkstra (Raw GraphBLAS)\n");
}


static void print_benchmark_header(const GraphInfo *graph_info, 
                                   GrB_Index source, 
                                   double delta) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║           SSSP GraphBLAS Benchmark v%-16s            ║\n", VERSION);
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    printf("║  Граф: %-58s  ║\n", graph_info->name);
    printf("║  Вершин: %-10llu  Рёбер: %-10llu  Источник: %-6llu            ║\n", 
           (unsigned long long)graph_info->nverts, (unsigned long long)graph_info->nedges, (unsigned long long)source);
    printf("║  Delta: %-59.2f  ║\n", delta);
    printf("╚════════════════════════════════════════════════════════════════╝\n");
}


static void print_results(SSSP_Result results[], int count) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                    РЕЗУЛЬТАТЫ БЕНЧМАРКА                        ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    printf("║  Алгоритм                      │  Время (мс)  │  Статус      ║\n");
    printf("╠═════════════════════════════════╪══════════════╪══════════════╣\n");
    
    for (int i = 0; i < count; i++) {
        char status[20];
        
        if (results[i].success) {
            snprintf(status, sizeof(status), "%s", "✅ Успех");
        } else {
            snprintf(status, sizeof(status), "%s", "❌ Ошибка");
        }
        
        double time_ms = results[i].success ? results[i].time_ms : 0.0;
        
        printf("║  %-32s │  %10.2f  │  %-11s ║\n", 
               results[i].name, 
               time_ms, 
               status);
    }
    
    printf("╚════════════════════════════════════════════════════════════════╝\n");
}


static void print_algorithm_stats(const SSSP_Result *result) {
    if (!result->success) {
        return;
    }
    
    printf("      ✅ Успех: %.2f мс", result->time_ms);
    
    if (result->iterations > 0) {
        printf(", итераций: %d", result->iterations);
    }
    
    printf(", достижимо: %llu", (unsigned long long)result->reachable_vertices);
    
    printf("\n");
}


static bool validate_results(SSSP_Result results[], int count) {
    if (count < 2) {
        return true;  /* Нечего сравнивать */
    }
    
    printf("\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("Валидация результатов:\n");
    
    bool all_valid = true;
    
    /* Используем первый алгоритм как эталон */
    SSSP_Result *reference = &results[0];
    
    for (int i = 1; i < count; i++) {
        if (!results[i].success) {
            continue;  /* Пропустить неудачные */
        }
        
        bool match = sssp_validate_distances(reference->distances, 
                                             results[i].distances);
        
        if (match) {
            printf("  ✅ %s совпадает с эталоном\n", results[i].name);
        } else {
            printf("  ❌ %s НЕ совпадает с эталоном!\n", results[i].name);
            all_valid = false;
        }
    }
    
    return all_valid;
}


static int find_best_algorithm(SSSP_Result results[], int count) {
    int best_idx = -1;
    double best_time = INFINITY;
    
    for (int i = 0; i < count; i++) {
        if (results[i].success && results[i].time_ms < best_time) {
            best_time = results[i].time_ms;
            best_idx = i;
        }
    }
    
    return best_idx;
}


static void print_speedup(SSSP_Result results[], int count, int best_idx) {
    if (best_idx < 0 || best_idx >= count) {
        return;
    }
    
    double best_time = results[best_idx].time_ms;
    
    if (best_time <= 0) {
        return;  /* Защита от деления на ноль */
    }
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  Ускорение (относительно %s):                           ║\n", 
           results[best_idx].name);
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    
    for (int i = 0; i < count; i++) {
        if (!results[i].success || results[i].time_ms <= 0) {
            continue;
        }
        
        double speedup = results[i].time_ms / best_time;
        
        if (i == best_idx) {
            printf("║  %-32s │  %8.2f×  (лучший)                      ║\n", 
                   results[i].name, speedup);
        } else {
            printf("║  %-32s │  %8.2f×                                ║\n", 
                   results[i].name, speedup);
        }
    }
    
    printf("╚════════════════════════════════════════════════════════════════╝\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *graph_file = argv[1];
    GrB_Index source = 0;
    double delta = 3.0;
    
    if (argc > 2) {
        char *endptr;
        long source_val = strtol(argv[2], &endptr, 10);
        if (*endptr == '\0' && source_val >= 0) {
            source = (GrB_Index)source_val;
        } else {
            fprintf(stderr, "⚠️  Некорректный источник: %s, используется 0\n", argv[2]);
        }
    }
    
    if (argc > 3) {
        char *endptr;
        double delta_val = strtod(argv[3], &endptr);
        if (*endptr == '\0' && delta_val > 0) {
            delta = delta_val;
        } else {
            fprintf(stderr, "⚠️  Некорректная delta: %s, используется 3.0\n", argv[3]);
        }
    }
    
    char msg[LAGRAPH_MSG_LEN];
    GrB_Info info = LAGraph_Init(msg);
    if (info != GrB_SUCCESS) {
        fprintf(stderr, "❌ LAGraph_Init failed: %d\n%s\n", info, msg);
        fprintf(stderr, "   Убедитесь, что GraphBLAS и LAGraph установлены\n");
        return 1;
    }
    
    LAGraph_Graph graph = NULL;
    GraphInfo graph_info;
    
    info = graph_load(&graph, graph_file, &graph_info);
    if (info != GrB_SUCCESS) {
        fprintf(stderr, "❌ Не удалось загрузить граф: %d\n", info);
        LAGraph_Finalize(msg);
        return 1;
    }
    
    if (source >= graph_info.nverts) {
        fprintf(stderr, "❌ Исходная вершина %llu вне диапазона [0, %llu)\n", 
                (unsigned long long)source, (unsigned long long)graph_info.nverts);
        LAGraph_Delete(&graph, msg);
        LAGraph_Finalize(msg);
        return 1;
    }
    
    print_benchmark_header(&graph_info, source, delta);
    
    SSSP_Result results[MAX_ALGORITHMS];
    int count = 0;
    

    printf("\n[1/3] LAGraph SSSP (Delta-Stepping)...\n");
    fflush(stdout);
    
    timer_start();
    info = lagraph_sssp(&results[count], graph, source, delta);
    results[count].time_ms = timer_stop_ms();
    
    if (info == GrB_SUCCESS) {
        print_algorithm_stats(&results[count]);
        count++;
    } else {
        printf("      ❌ Ошибка: %d\n", info);
    }
    

    printf("\n[2/3] Algebraic Bellman-Ford...\n");
    fflush(stdout);
    
    timer_start();
    info = algebraic_bf_graphblas(&results[count], graph, source, 0.0);
    results[count].time_ms = timer_stop_ms();
    
    if (info == GrB_SUCCESS) {
        print_algorithm_stats(&results[count]);
        count++;
    } else {
        printf("      ❌ Ошибка: %d\n", info);
    }
    

    printf("\n[3/3] Dijkstra...\n");
    fflush(stdout);
    
    timer_start();
    info = dijkstra_graphblas(&results[count], graph, source, 0.0);
    results[count].time_ms = timer_stop_ms();
    
    if (info == GrB_SUCCESS) {
        print_algorithm_stats(&results[count]);
        count++;
    } else {
        printf("      ❌ Ошибка: %d\n", info);
    }
    
    print_results(results, count);
    
    bool validation_passed = validate_results(results, count);
    
    int best_idx = find_best_algorithm(results, count);
    if (best_idx >= 0) {
        print_speedup(results, count, best_idx);
    }
    
    for (int i = 0; i < count; i++) {
        sssp_result_cleanup(&results[i]);
    }
    
    LAGraph_Delete(&graph, msg);
    
    info = LAGraph_Finalize(msg);
    if (info != GrB_SUCCESS) {
        fprintf(stderr, "⚠️  LAGraph_Finalize warning: %d\n%s\n", info, msg);
    }
    
    if (count == 0) {
        fprintf(stderr, "❌ Ни один алгоритм не выполнился успешно\n");
        return 1;
    }
    
    if (!validation_passed) {
        fprintf(stderr, "⚠️  Валидация не пройдена: результаты алгоритмов различаются\n");
    }
    
    return 0;
}