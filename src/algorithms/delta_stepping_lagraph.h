#ifndef DELTA_STEPPING_LAGRAPH_H
#define DELTA_STEPPING_LAGRAPH_H

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
    double delta;
} Delta_Result;

GrB_Info delta_stepping_lagraph(Delta_Result *result, LAGraph_Graph graph, GrB_Index source, double delta);
void delta_result_cleanup(Delta_Result *result);

#endif