/**
 * @file algebraic_bf_graphblas.c
 * @brief Реализация Algebraic Bellman-Ford через min-plus алгебру
 *
 * Математическая основа:
 * d^(k+1) = d^(k) ⊗ A  где ⊗ - умножение в min-plus алгебре
 *
 */

#include "algebraic_bf_graphblas.h"

GrB_Info algebraic_bf_graphblas(SSSP_Result *result, LAGraph_Graph graph, GrB_Index source,
                                double delta) {
    (void)delta;

    if (!result || !graph) {
        return GrB_NULL_POINTER;
    }

    sssp_result_init(result, "Algebraic BF (GraphBLAS)", "Raw GraphBLAS");

    GrB_Index n;
    GrB_Matrix_nrows(&n, graph->A);
    result->vertices_processed = n;

    GrB_Monoid min_monoid = NULL;
    GrB_Semiring minplus_semiring = NULL;
    GrB_Vector dtmp = NULL;
    GrB_Vector improved = NULL;
    GrB_Info info;

    info = GrB_Vector_new(&result->distances, GrB_FP64, n);
    if (info != GrB_SUCCESS)
        goto cleanup;

    info = GrB_Vector_new(&result->predecessors, GrB_UINT64, n);
    if (info != GrB_SUCCESS)
        goto cleanup;

    GrB_Vector_setElement(result->distances, 0.0, source);
    GrB_Vector_setElement(result->predecessors, source, source);

    info = GrB_Monoid_new(&min_monoid, GrB_MIN_FP64, (double)INFINITY);
    if (info != GrB_SUCCESS)
        goto cleanup;

    info = GrB_Semiring_new(&minplus_semiring, min_monoid, GrB_PLUS_FP64);
    if (info != GrB_SUCCESS)
        goto cleanup;

    info = GrB_Vector_new(&dtmp, GrB_FP64, n);
    if (info != GrB_SUCCESS)
        goto cleanup;

    info = GrB_Vector_new(&improved, GrB_BOOL, n);
    if (info != GrB_SUCCESS)
        goto cleanup;

    GrB_Index old_nvals;
    GrB_Index new_nvals;

    for (GrB_Index k = 1; k < n; k++) {
        result->iterations = k;

        info = GrB_vxm(dtmp, NULL, NULL, minplus_semiring, result->distances, graph->A, NULL);
        if (info != GrB_SUCCESS)
            break;

        GrB_Index dtmp_nvals;
        GrB_Vector_nvals(&dtmp_nvals, dtmp);
        if (dtmp_nvals == 0)
            break;

        GrB_Vector_nvals(&old_nvals, result->distances);

        GrB_eWiseMult(improved, NULL, NULL, GrB_LT_FP64, dtmp, result->distances, NULL);

        info = GrB_eWiseAdd(result->distances, NULL, NULL, GrB_MIN_FP64, result->distances, dtmp,
                            NULL);
        if (info != GrB_SUCCESS)
            break;

        GrB_Vector_nvals(&new_nvals, result->distances);
        if (new_nvals == old_nvals) {
            bool any_improvement = false;
            GrB_reduce(&any_improvement, NULL, GrB_LOR_MONOID_BOOL, improved, NULL);
            if (!any_improvement)
                break;
        }
    }

    GrB_Vector_nvals(&result->reachable_vertices, result->distances);
    result->success = true;

cleanup:
    GrB_free(&improved);
    GrB_free(&dtmp);
    GrB_free(&minplus_semiring);
    GrB_free(&min_monoid);

    return info;
}
