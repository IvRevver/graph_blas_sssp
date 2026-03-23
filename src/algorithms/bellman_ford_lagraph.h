#ifndef BELLMAN_FORD_LAGRAPH_H
#define BELLMAN_FORD_LAGRAPH_H

#include <stdbool.h>
#include "GraphBLAS.h"
#include "LAGraph.h"

typedef struct {
    char name[64];
    double time_ms;
    int iterations;
    bool success;
    GrB_Index vertices_processed;
    GrB_Index reachable_vertices;
    GrB_Vector distances;
    GrB_Vector predecessors;
} BF_Result;

GrB_Info bellman_ford_lagraph(BF_Result *result, LAGraph_Graph graph, GrB_Index source);
void bf_result_cleanup(BF_Result *result);

#endif