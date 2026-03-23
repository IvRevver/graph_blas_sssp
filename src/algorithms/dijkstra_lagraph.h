#ifndef DIJKSTRA_LAGRAPH_H
#define DIJKSTRA_LAGRAPH_H

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
} Dijkstra_Result;

GrB_Info dijkstra_lagraph(Dijkstra_Result *result, LAGraph_Graph graph, GrB_Index source);
void dijkstra_result_cleanup(Dijkstra_Result *result);

#endif