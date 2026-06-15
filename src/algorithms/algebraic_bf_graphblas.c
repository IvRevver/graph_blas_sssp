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
    
    GrB_Semiring minplus_semiring = NULL;
    GrB_Matrix AT = NULL;
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
    
    info = GrB_Semiring_new(&minplus_semiring, GrB_MIN_FP64, GrB_PLUS_FP64, GrB_FP64);
    if (info != GrB_SUCCESS) goto cleanup;
    
    info = GrB_Matrix_new(&AT, GrB_FP64, n, n);
    if (info != GrB_SUCCESS) goto cleanup;
    
    info = GrB_transpose(AT, NULL, NULL, graph->A, NULL);
    if (info != GrB_SUCCESS) goto cleanup;
    
    info = GrB_Vector_new(&dtmp, GrB_FP64, n);
    if (info != GrB_SUCCESS) goto cleanup;
    
    /* Основной цикл Bellman-Ford: n-1 итераций */
    bool converged = false;
    for (GrB_Index k = 1; k < n; k++) {
        result->iterations = k;
        
        /* dtmp = d min.plus A (векторно-матричное умножение) */
        info = GrB_assign(dtmp, NULL, NULL, result->distances, NULL);
        if (info != GrB_SUCCESS) break;
        
        info = GrB_vxm(dtmp, NULL, NULL, minplus_semiring, 
                       result->distances, AT, NULL);
        if (info != GrB_SUCCESS) break;
        
        /* Проверка сходимости */
        converged = true;
        for (GrB_Index i = 0; i < n; i++) {
            double d1, d2;
            GrB_Info info1 = GrB_Vector_extractElement(&d1, result->distances, i);
            GrB_Info info2 = GrB_Vector_extractElement(&d2, dtmp, i);
            
            if (info1 == GrB_SUCCESS && info2 == GrB_SUCCESS) {
                if (d1 != d2) {
                    converged = false;
                    break;
                }
            }
        }
        
        /* d = dtmp */
        info = GrB_assign(result->distances, NULL, NULL, dtmp, NULL);
        if (info != GrB_SUCCESS) break;
        
        if (converged) {
            break;
        }
    }
    
    GrB_Vector_nvals(&result->reachable_vertices, result->distances);
    result->success = true;
    result->iterations = converged ? result->iterations : n - 1;

cleanup:
    GrB_free(&dtmp);
    GrB_free(&AT);
    GrB_free(&minplus_semiring);
    
    return info;
}
