#include "delta_stepping_lagraph.h"
#include <time.h>
#include <string.h>
#include <math.h>

GrB_Info delta_stepping_lagraph(Delta_Result *result, LAGraph_Graph graph, GrB_Index source, double delta) {
    if (!result || !graph) return GrB_NULL_POINTER;
    if (delta <= 0) return GrB_INVALID_VALUE;
    
    memset(result, 0, sizeof(Delta_Result));
    strcpy(result->name, "Delta-Stepping (LAGraph)");
    result->delta = delta;
    
    GrB_Index n = graph->nrows;
    result->vertices_processed = n;
    
    clock_t start = clock();
    
    GrB_Info info = LAGraph_DeltaStepping(
        &result->distances,
        &result->predecessors,
        graph,
        source,
        delta,
        NULL
    );
    
    clock_t end = clock();
    result->time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    if (info == GrB_SUCCESS) {
        result->success = true;
        GrB_Vector_nvals(&result->reachable_vertices, result->distances);
        result->iterations = (int)ceil(n / delta);
    }
    
    return info;
}

void delta_result_cleanup(Delta_Result *result) {
    if (!result) return;
    if (result->distances) GrB_free(&result->distances);
    if (result->predecessors) GrB_free(&result->predecessors);
}