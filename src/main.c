#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

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

#define CSV_DIR "results"


static void ensure_results_dir(void) {
#ifdef _WIN32
    if (_mkdir(CSV_DIR) != 0 && errno != EEXIST)
#else
    if (mkdir(CSV_DIR, 0755) != 0 && errno != EEXIST)
#endif
        fprintf(stderr, "[!] Could not create %s directory\n", CSV_DIR);
}


static void sanitize_filename(const char *src, char *dst, size_t dst_size) {
    size_t i = 0;
    while (*src && i < dst_size - 1) {
        char c = *src++;
        if (strchr("<>:\"/\\|?* ", c))
            c = '_';
        dst[i++] = c;
    }
    dst[i] = '\0';
}


static void write_csv_table(const char *graph_name, const char *algorithms[],
                             double means[], double mins[], double maxs[],
                             double stds[], int count) {
    char safe_name[256];
    sanitize_filename(graph_name, safe_name, sizeof(safe_name));

    char path[512];
    snprintf(path, sizeof(path), CSV_DIR "/%s.csv", safe_name);

    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "[!] Could not open %s for writing\n", path);
        return;
    }

    fprintf(f, "graph,algorithm,avg_ms,min_ms,max_ms,stddev\n");
    for (int i = 0; i < count; i++) {
        fprintf(f, "%s,%s,%.2f,%.2f,%.2f,%.2f\n",
                graph_name, algorithms[i], means[i], mins[i], maxs[i], stds[i]);
    }

    fclose(f);
}


static void print_usage(const char *prog) {
    printf("Usage: %s <file.mtx> [source] [delta] [runs]\n", prog);
    printf("       %s <file.mtx> -b [source] [runs]\n", prog);
    printf("\n");
    printf("Arguments:\n");
    printf("  <file.mtx>    Path to graph in Matrix Market format\n");
    printf("  source        Source vertex (default: 0)\n");
    printf("  delta         Delta parameter for Delta-Stepping (default: 3.0)\n");
    printf("  runs          Number of iterations for stats benchmark (default: 1)\n");
    printf("  -b            Full benchmark: all deltas x all algorithms x runs\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s graphs/test.mtx\n", prog);
    printf("  %s graphs/test.mtx 0 3.0\n", prog);
    printf("  %s graphs/test.mtx 0 3.0 30\n", prog);
    printf("  %s graphs/astro-ph.mtx -b\n", prog);
    printf("  %s graphs/astro-ph.mtx -b 0 30\n", prog);
}


static void print_benchmark_header(const GraphInfo *graph_info, 
                                   GrB_Index source, 
                                   double delta) {
    char line[256];
    printf("+--------------------------------------------------------------+\n");
    snprintf(line, sizeof(line), "SSSP GraphBLAS Benchmark v%s", VERSION);
    printf("| %-60s |\n", line);
    printf("|--------------------------------------------------------------|\n");
    snprintf(line, sizeof(line), "Graph: %.248s", graph_info->name);
    printf("| %-60s |\n", line);
    snprintf(line, sizeof(line), "Vertices: %llu  Edges: %llu  Source: %llu", 
             (unsigned long long)graph_info->nverts,
             (unsigned long long)graph_info->nedges,
             (unsigned long long)source);
    printf("| %-60s |\n", line);
    snprintf(line, sizeof(line), "Delta: %.2f", delta);
    printf("| %-60s |\n", line);
    printf("+--------------------------------------------------------------+\n");
}


static void print_results(SSSP_Result results[], int count) {
    printf("\n");
    printf("+--------------------------------------------------------------+\n");
    printf("| %-60s |\n", "BENCHMARK RESULTS");
    printf("+----------------------------+--------------+------------------+\n");
    printf("| %-26s | %-12s | %-16s |\n", "Algorithm", "Time (ms)", "Status");
    printf("+----------------------------+--------------+------------------+\n");
    
    for (int i = 0; i < count; i++) {
        char status[20];
        
        if (results[i].success) {
            snprintf(status, sizeof(status), "%s", "OK");
        } else {
            snprintf(status, sizeof(status), "%s", "FAIL");
        }
        
        double time_ms = results[i].success ? results[i].time_ms : 0.0;
        
        printf("| %-26s | %12.2f | %-16s |\n", 
               results[i].name, 
               time_ms, 
               status);
    }
    
    printf("+--------------------------------------------------------------+\n");
}


static void print_algorithm_stats(const SSSP_Result *result) {
    if (!result->success) {
        return;
    }
    
    printf("      [OK] %.2f ms", result->time_ms);
    
    if (result->iterations > 0) {
        printf(", iters: %d", result->iterations);
    }
    
    printf(", reachable: %llu", (unsigned long long)result->reachable_vertices);
    
    printf("\n");
}


static bool validate_results(SSSP_Result results[], int count) {
    if (count < 2) {
        return true;  /* Нечего сравнивать */
    }
    
    printf("\n");
    printf("===============================================================\n");
    printf("Validation:\n");
    
    bool all_valid = true;
    
    SSSP_Result *reference = &results[0];
    
    for (int i = 1; i < count; i++) {
        if (!results[i].success) {
            continue;
        }
        
        bool match = sssp_validate_distances(reference->distances, 
                                             results[i].distances);
        
        if (match) {
            printf("  [OK] %s matches reference\n", results[i].name);
        } else {
            printf("  [FAIL] %s does NOT match reference!\n", results[i].name);
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
        return;
    }
    
    char line[256];
    printf("\n");
    printf("+--------------------------------------------------------------+\n");
            snprintf(line, sizeof(line), "Speedup (vs %.63s)", results[best_idx].name);
    printf("| %-60s |\n", line);
    printf("+----------------------------+---------------------------------+\n");
    
    for (int i = 0; i < count; i++) {
        if (!results[i].success || results[i].time_ms <= 0) {
            continue;
        }
        
        double speedup = results[i].time_ms / best_time;
        
    char line[256];
        if (i == best_idx) {
            snprintf(line, sizeof(line), "%-28.63s %s %8.2fx  (fastest)", 
                     results[i].name, "|", speedup);
        } else {
            snprintf(line, sizeof(line), "%-28.63s %s %8.2fx", 
                     results[i].name, "|", speedup);
        }
        printf("| %-60s |\n", line);
    }
    
    printf("+--------------------------------------------------------------+\n");
}

static void print_stats_header(void) {
    printf("\n+---------------------+----------+----------+----------+----------+\n");
    printf("| Algorithm           | Avg (ms) | Min (ms) | Max (ms) | StdDev   |\n");
    printf("+---------------------+----------+----------+----------+----------+\n");
}

static void print_stats_row(const char *name, double mean, double min, double max, double std) {
    printf("| %-20s | %8.2f | %8.2f | %8.2f | %8.2f |\n",
           name, mean, min, max, std);
}

static void run_stats_benchmark(LAGraph_Graph graph, GraphInfo *graph_info,
                                 GrB_Index source, double delta, int runs) {
    double *times = malloc(runs * sizeof(double));
    if (!times) {
        fprintf(stderr, "[!] Out of memory for %d runs\n", runs);
        return;
    }

    typedef GrB_Info (*AlgFunc)(SSSP_Result*, LAGraph_Graph, GrB_Index, double);

    struct { const char *name; AlgFunc func; double param; bool skip_negative; } algs[] = {
        { "Delta-Stepping (LAG)", lagraph_sssp,           delta, true },
        { "Algebraic BF (GB)",    algebraic_bf_graphblas,  0.0,   false },
        { "Dijkstra (GB)",       dijkstra_graphblas,      0.0,   true },
    };
    int nalgs = sizeof(algs) / sizeof(algs[0]);

    const char *names[16];
    double means[16], mins[16], maxs[16], stds[16];
    bool succeeded[16];
    int count = 0;

    for (int a = 0; a < nalgs; a++) {
        if (algs[a].skip_negative && graph_info->has_negative_weights) {
            continue;
        }

        int valid = 0;
        for (int i = 0; i < runs; i++) {
            SSSP_Result result;
            timer_start();
            GrB_Info info = algs[a].func(&result, graph, source, algs[a].param);
            double t = timer_stop_ms();
            if (info == GrB_SUCCESS) {
                times[valid++] = t;
            }
            sssp_result_cleanup(&result);
        }

        succeeded[count] = (valid > 0);

        if (valid >= 1) {
            double sum = 0, mn = times[0], mx = times[0];
            for (int i = 0; i < valid; i++) {
                sum += times[i];
                if (times[i] < mn) mn = times[i];
                if (times[i] > mx) mx = times[i];
            }
            double mean = sum / valid;
            double sq = 0;
            for (int i = 0; i < valid; i++) {
                double d = times[i] - mean;
                sq += d * d;
            }
            double std = sqrt(sq / valid);

            means[count] = mean;
            mins[count] = mn;
            maxs[count] = mx;
            stds[count] = std;
        } else {
            means[count] = mins[count] = maxs[count] = stds[count] = 0.0;
        }

        names[count] = algs[a].name;
        count++;
    }

    printf("\n[!] Statistical benchmark: %d runs per algorithm\n", runs);
    print_stats_header();
    for (int i = 0; i < count; i++) {
        if (succeeded[i]) {
            print_stats_row(names[i], means[i], mins[i], maxs[i], stds[i]);
        } else {
            printf("| %-20s |    FAIL    |    FAIL    |    FAIL    |    FAIL    |\n", names[i]);
        }
    }
    printf("+---------------------+----------+----------+----------+----------+\n");

    ensure_results_dir();
    {
        const char *csv_names[16];
        double csv_means[16], csv_mins[16], csv_maxs[16], csv_stds[16];
        int csv_count = 0;
        for (int i = 0; i < count; i++) {
            if (succeeded[i] || (count == 1)) {
                csv_names[csv_count] = names[i];
                csv_means[csv_count] = means[i];
                csv_mins[csv_count] = mins[i];
                csv_maxs[csv_count] = maxs[i];
                csv_stds[csv_count] = stds[i];
                csv_count++;
            }
        }
        if (csv_count > 0)
            write_csv_table(graph_info->name, csv_names, csv_means, csv_mins, csv_maxs, csv_stds, csv_count);
    }

    free(times);
}

static bool compute_stats(
                           GrB_Info (*func)(SSSP_Result*, LAGraph_Graph, GrB_Index, double),
                           double param, LAGraph_Graph graph, GrB_Index source,
                           int runs, bool skip_negative, GraphInfo *graph_info,
                           double *out_mean, double *out_min, double *out_max, double *out_std) {
    if (skip_negative && graph_info->has_negative_weights) return false;

    double *times = malloc(runs * sizeof(double));
    if (!times) return false;

    int valid = 0;
    for (int i = 0; i < runs; i++) {
        SSSP_Result result;
        timer_start();
        GrB_Info info = func(&result, graph, source, param);
        double t = timer_stop_ms();
        if (info == GrB_SUCCESS) times[valid++] = t;
        sssp_result_cleanup(&result);
    }

    if (valid < 1) { free(times); return false; }

    double sum = 0, mn = times[0], mx = times[0];
    for (int i = 0; i < valid; i++) {
        sum += times[i];
        if (times[i] < mn) mn = times[i];
        if (times[i] > mx) mx = times[i];
    }
    *out_mean = sum / valid;
    *out_min = mn;
    *out_max = mx;
    double sq = 0;
    for (int i = 0; i < valid; i++) {
        double d = times[i] - *out_mean;
        sq += d * d;
    }
    *out_std = sqrt(sq / valid);

    free(times);
    return true;
}


static void run_full_benchmark(LAGraph_Graph graph, GraphInfo *graph_info,
                                GrB_Index source, int runs) {
    const double deltas[] = {0.5, 1.0, 2.0, 3.0, 5.0, 10.0};
    int ndeltas = sizeof(deltas) / sizeof(deltas[0]);

    char labels[20][64];
    double means[20], mins[20], maxs[20], stds[20];
    int n = 0;

    for (int d = 0; d < ndeltas; d++) {
        if (d == 0) {
            double m, mn, mx, s;
            if (compute_stats(algebraic_bf_graphblas, 0.0,
                              graph, source, runs, false, graph_info,
                              &m, &mn, &mx, &s)) {
                snprintf(labels[n], sizeof(labels[n]), "Algebraic BF");
                means[n] = m; mins[n] = mn; maxs[n] = mx; stds[n] = s;
                n++;
            }
            if (compute_stats(dijkstra_graphblas, 0.0,
                              graph, source, runs, true, graph_info,
                              &m, &mn, &mx, &s)) {
                snprintf(labels[n], sizeof(labels[n]), "Dijkstra");
                means[n] = m; mins[n] = mn; maxs[n] = mx; stds[n] = s;
                n++;
            }
        }

        char ds_label[64];
        snprintf(ds_label, sizeof(ds_label), "DS delta=%.1f", deltas[d]);
        double m, mn, mx, s;
        if (compute_stats(lagraph_sssp, deltas[d],
                          graph, source, runs, true, graph_info,
                          &m, &mn, &mx, &s)) {
            snprintf(labels[n], sizeof(labels[n]), "%s", ds_label);
            means[n] = m; mins[n] = mn; maxs[n] = mx; stds[n] = s;
            n++;
        }
    }

    printf("\n+-- Full benchmark: %s (source=%llu, runs=%d) --+\n",
           graph_info->name, (unsigned long long)source, runs);
    printf("| %-30s | %8s | %8s | %8s | %8s |\n",
           "Algorithm", "Avg(ms)", "Min(ms)", "Max(ms)", "StdDev");
    printf("+--------------------------------+----------+----------+----------+----------+\n");
    for (int i = 0; i < n; i++) {
        printf("| %-30s | %8.2f | %8.2f | %8.2f | %8.2f |\n",
               labels[i], means[i], mins[i], maxs[i], stds[i]);
    }
    printf("+--------------------------------+----------+----------+----------+----------+\n");

    const char *alg_names[20];
    for (int i = 0; i < n; i++) alg_names[i] = labels[i];
    ensure_results_dir();
    write_csv_table(graph_info->name, alg_names, means, mins, maxs, stds, n);
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *graph_file = argv[1];
    GrB_Index source = 0;
    double delta = 3.0;
    int runs = 1;
    bool full_bench = false;
    int arg_idx = 2;

    if (argc > 2 && (strcmp(argv[2], "-b") == 0 || strcmp(argv[2], "--full-bench") == 0)) {
        full_bench = true;
        arg_idx = 3;
        delta = 0.0;
    }

    if (argc > arg_idx) {
        char *endptr;
        long source_val = strtol(argv[arg_idx], &endptr, 10);
        if (*endptr == '\0' && source_val >= 0) {
            source = (GrB_Index)source_val;
            arg_idx++;
        } else {
            fprintf(stderr, "[!] Invalid source: %s, using 0\n", argv[arg_idx]);
            arg_idx++;
        }
    }

    if (!full_bench && argc > arg_idx) {
        char *endptr;
        double delta_val = strtod(argv[arg_idx], &endptr);
        if (*endptr == '\0' && delta_val > 0) {
            delta = delta_val;
            arg_idx++;
        } else {
            fprintf(stderr, "[!] Invalid delta: %s, using 3.0\n", argv[arg_idx]);
            arg_idx++;
        }
    }

    if (argc > arg_idx) {
        char *endptr;
        long runs_val = strtol(argv[arg_idx], &endptr, 10);
        if (*endptr == '\0' && runs_val >= 1) {
            runs = (int)runs_val;
        } else {
            fprintf(stderr, "[!] Invalid runs: %s, using 1\n", argv[arg_idx]);
        }
    }
    
    char msg[LAGRAPH_MSG_LEN];
    GrB_Info info = LAGraph_Init(msg);
    if (info != GrB_SUCCESS) {
        fprintf(stderr, "[FAIL] LAGraph_Init failed: %d\n%s\n", info, msg);
        fprintf(stderr, "   убедитесь, что GraphBLAS и LAGraph установлены\n");
        return 1;
    }
    
    LAGraph_Graph graph = NULL;
    GraphInfo graph_info;
    
    info = graph_load(&graph, graph_file, &graph_info);
    if (info != GrB_SUCCESS) {
        fprintf(stderr, "[FAIL] Could not load graph: %d\n", info);
        LAGraph_Finalize(msg);
        return 1;
    }
    
    if (source >= graph_info.nverts) {
        fprintf(stderr, "[FAIL] Source vertex %llu out of range [0, %llu)\n", 
                (unsigned long long)source, (unsigned long long)graph_info.nverts);
        LAGraph_Delete(&graph, msg);
        LAGraph_Finalize(msg);
        return 1;
    }

    if (full_bench) {
        if (runs < 2) runs = 30;
        run_full_benchmark(graph, &graph_info, source, runs);
        LAGraph_Delete(&graph, msg);
        LAGraph_Finalize(msg);
        return 0;
    }

    if (runs > 1) {
        print_benchmark_header(&graph_info, source, delta);
        run_stats_benchmark(graph, &graph_info, source, delta, runs);
        LAGraph_Delete(&graph, msg);
        LAGraph_Finalize(msg);
        return 0;
    }
    
    print_benchmark_header(&graph_info, source, delta);
    
    SSSP_Result results[MAX_ALGORITHMS];
    int count = 0;
    int step = 1;

    if (graph_info.has_negative_weights) {
        printf("\n[!] Graph has negative weights — skipping Delta-Stepping (LAGraph) and Dijkstra\n");
    }

    if (!graph_info.has_negative_weights) {
        printf("\n[%d/3] LAGraph SSSP (Delta-Stepping)...\n", step++);
        fflush(stdout);
        
        timer_start();
        info = lagraph_sssp(&results[count], graph, source, delta);
        results[count].time_ms = timer_stop_ms();
        
        if (info == GrB_SUCCESS) {
            print_algorithm_stats(&results[count]);
            count++;
        } else {
            printf("      [FAIL] error: %d\n", info);
        }
    }
    

    printf("\n[%d/3] Algebraic Bellman-Ford...\n", step++);
    fflush(stdout);
    
    timer_start();
    info = algebraic_bf_graphblas(&results[count], graph, source, 0.0);
    results[count].time_ms = timer_stop_ms();
    
    if (info == GrB_SUCCESS) {
        print_algorithm_stats(&results[count]);
        count++;
    } else {
        printf("      [FAIL] error: %d\n", info);
    }
    

    if (!graph_info.has_negative_weights) {
        printf("\n[%d/3] Dijkstra...\n", step++);
        fflush(stdout);
        
        timer_start();
        info = dijkstra_graphblas(&results[count], graph, source, 0.0);
        results[count].time_ms = timer_stop_ms();
        
        if (info == GrB_SUCCESS) {
            print_algorithm_stats(&results[count]);
            count++;
        } else {
            printf("      [FAIL] error: %d\n", info);
        }
    }
    
    print_results(results, count);

    {
        const char *names[16];
        double means[16], mins[16], maxs[16], stds[16];
        int csv_count = 0;
        for (int i = 0; i < count; i++) {
            names[csv_count] = results[i].name;
            means[csv_count] = results[i].time_ms;
            mins[csv_count] = results[i].time_ms;
            maxs[csv_count] = results[i].time_ms;
            stds[csv_count] = 0.0;
            csv_count++;
        }
        ensure_results_dir();
        write_csv_table(graph_info.name, names, means, mins, maxs, stds, csv_count);
    }

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
        fprintf(stderr, "[!] LAGraph_Finalize warning: %d\n%s\n", info, msg);
    }
    
    if (count == 0) {
        fprintf(stderr, "[FAIL] No algorithm succeeded\n");
        return 1;
    }
    
    if (!validation_passed) {
        fprintf(stderr, "[!] Validation failed: results differ\n");
    }
    
    return 0;
}