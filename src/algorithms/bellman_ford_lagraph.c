#include "bellman_ford_lagraph.h"
#include <time.h>
#include <string.h>

GrB_Info bellman_ford_lagraph(BF_Result *result, LAGraph_Graph graph, GrB_Index source) {
    if (!result || !graph) return GrB_NULL_POINTER;
    
    memset(result, 0, sizeof(BF_Result));
    strcpy(result->name, "Bellman-Ford (LAGraph)");
    
    GrB_Index n = graph->nrows;
    result->vertices_processed = n;
    
    clock_t start = clock();
    
    GrB_Info info = LAGraph_BellmanFord(
        &result->distances,
        &result->predecessors,
        graph,
        source,
        NULL
    );
    
    clock_t end = clock();
    result->time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    if (info == GrB_SUCCESS) {
        result->success = true;
        GrB_Vector_nvals(&result->reachable_vertices, result->distances);
        result->iterations = n - 1;
    }
    
    return info;
}

void bf_result_cleanup(BF_Result *result) {
    if (!result) return;
    if (result->distances) GrB_free(&result->distances);
    if (result->predecessors) GrB_free(&result->predecessors);
}