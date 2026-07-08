/**
 * @file lagraph_sssp.c
 * @brief Реализация через LAGr_SingleSourceShortestPath (Delta-Stepping)
 */

#include "lagraph_sssp.h"

GrB_Info lagraph_sssp(SSSP_Result *result, LAGraph_Graph graph, GrB_Index source, double delta) {
    if (!result || !graph) {
        return GrB_NULL_POINTER;
    }

    if (delta <= 0) {
        return GrB_INVALID_VALUE;
    }

    sssp_result_init(result, "Delta-Stepping (LAGraph)", "LAGraph stable");

    GrB_Index n;
    GrB_Matrix_nrows(&n, graph->A);
    result->vertices_processed = n;

    char msg[LAGRAPH_MSG_LEN];

    GrB_Scalar delta_scalar = NULL;
    GrB_Scalar_new(&delta_scalar, GrB_FP64);
    GrB_Scalar_setElement(delta_scalar, delta);

    GrB_Info info =
        LAGr_SingleSourceShortestPath(&result->distances, graph, source, delta_scalar, msg);

    GrB_free(&delta_scalar);

    if (info == GrB_SUCCESS) {
        result->success = true;
        GrB_Vector_nvals(&result->reachable_vertices, result->distances);
        result->iterations = (int)ceil((double)n / delta);
    }

    return info;
}
