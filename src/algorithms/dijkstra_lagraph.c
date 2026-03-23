#include "dijkstra_lagraph.h"
#include <time.h>
#include <string.h>
#include <math.h>

static bool has_negative_weights(LAGraph_Graph graph) {
    GrB_Matrix A = graph->A;
    GrB_Index nvals;
    GrB_Matrix_nvals(&nvals, A);
    
    GrB_Index *I = malloc(sizeof(GrB_Index) * nvals);
    GrB_Index *J = malloc(sizeof(GrB_Index) * nvals);
    double *X = malloc(sizeof(double) * nvals);
    
    GrB_Matrix_extractTuples(I, J, X, &nvals, A);
    
    bool has_negative = false;
    for (GrB_Index i = 0; i < nvals; i++) {
        if (X[i] < 0) {
            has_negative = true;
            break;
        }
    }
    
    free(I); free(J); free(X);
    return has_negative;
}

GrB_Info dijkstra_lagraph(Dijkstra_Result *result, LAGraph_Graph graph, GrB_Index source) {
    if (!result || !graph) return GrB_NULL_POINTER;
    
    memset(result, 0, sizeof(Dijkstra_Result));
    strcpy(result->name, "Dijkstra (LAGraph)");
    
    if (has_negative_weights(graph)) {
        result->success = false;
        return GrB_INVALID_VALUE;
    }
    
    GrB_Index n = graph->nrows;
    result->vertices_processed = n;
    
    clock_t start = clock();
    
    GrB_Info info = LAGraph_SSSP(
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
        result->iterations = result->reachable_vertices;
    }
    
    return info;
}

void dijkstra_result_cleanup(Dijkstra_Result *result) {
    if (!result) return;
    if (result->distances) GrB_free(&result->distances);
    if (result->predecessors) GrB_free(&result->predecessors);
}