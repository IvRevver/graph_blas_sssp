/**
 * @file algebraic_bf_graphblas.c
 * @brief Реализация Algebraic Bellman-Ford через min-plus алгебру
 * 
 * Математическая основа:
 * d^(k+1) = d^(k) ⊗ A  где ⊗ - умножение в min-plus алгебре
 * 
 */

#include "algebraic_bf_graphblas.h"

GrB_Info algebraic_bf_graphblas(SSSP_Result *result, LAGraph_Graph graph, 
                                GrB_Index source, double delta) {
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
    GrB_Info info;
    
    info = GrB_Vector_new(&result->distances, GrB_FP64, n);
    if (info != GrB_SUCCESS) goto cleanup;
    
    info = GrB_Vector_new(&result->predecessors, GrB_UINT64, n);
    if (info != GrB_SUCCESS) goto cleanup;
    
    for (GrB_Index i = 0; i < n; i++) {
        GrB_Vector_setElement(result->distances, INF, i);
        GrB_Vector_setElement(result->predecessors, (uint64_t)-1, i);
    }
    GrB_Vector_setElement(result->distances, 0.0, source);
    GrB_Vector_setElement(result->predecessors, source, source);
    
    info = GrB_Monoid_new(&min_monoid, GrB_MIN_FP64, (double)INFINITY);
    if (info != GrB_SUCCESS) goto cleanup;

    info = GrB_Semiring_new(&minplus_semiring, min_monoid, GrB_PLUS_FP64);
    if (info != GrB_SUCCESS) goto cleanup;
    
    info = GrB_Vector_new(&dtmp, GrB_FP64, n);
    if (info != GrB_SUCCESS) goto cleanup;
    
    /* Основной цикл Bellman-Ford: n-1 итераций */
    bool converged = false;
    for (GrB_Index k = 1; k < n; k++) {
        result->iterations = k;
        
        /* dtmp = distances ⊗ A (min-plus векторно-матричное умножение) */
        info = GrB_vxm(dtmp, NULL, NULL, minplus_semiring, 
                       result->distances, graph->A, NULL);
        if (info != GrB_SUCCESS) break;
        
        /* Проверка сходимости и копирование dtmp → distances */
        converged = true;
        for (GrB_Index i = 0; i < n; i++) {
            double d_old, d_new;
            GrB_Info info_old = GrB_Vector_extractElement(&d_old, result->distances, i);
            GrB_Info info_new = GrB_Vector_extractElement(&d_new, dtmp, i);
            
            if (info_new == GrB_SUCCESS) {
                /* dtmp has a value for this vertex — update distances */
                GrB_Vector_setElement(result->distances, d_new, i);
                if (info_old != GrB_SUCCESS || d_old != d_new) {
                    converged = false;
                }
            }
        }
        
        if (converged) {
            break;
        }
    }
    
    GrB_Vector_nvals(&result->reachable_vertices, result->distances);
    result->success = true;
    result->iterations = converged ? result->iterations : (int)(n - 1);

cleanup:
    GrB_free(&dtmp);
    GrB_free(&minplus_semiring);
    GrB_free(&min_monoid);
    
    return info;
}
