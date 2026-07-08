/**
 * @file dijkstra_graphblas.c
 * @brief Реализация Dijkstra с приоритетной очередью
 *
 * Математическая основа:
 * Жадный выбор вершины с минимальным расстоянием
 * Релаксация рёбер через GraphBLAS vxm
 *
 */

#include "dijkstra_graphblas.h"
#include <limits.h>

/**
 * @brief Простая приоритетная очередь (min-heap)
 */
typedef struct {
    GrB_Index *vertices;
    double *priorities;
    GrB_Index size;
    GrB_Index capacity;
} PriorityQueue;

static PriorityQueue *pq_create(GrB_Index capacity) {
    PriorityQueue *pq = malloc(sizeof(PriorityQueue));
    if (!pq)
        return NULL;

    pq->vertices = malloc(sizeof(GrB_Index) * capacity);
    pq->priorities = malloc(sizeof(double) * capacity);

    if (!pq->vertices || !pq->priorities) {
        free(pq->vertices);
        free(pq->priorities);
        free(pq);
        return NULL;
    }

    pq->size = 0;
    pq->capacity = capacity;
    return pq;
}

static void pq_free(PriorityQueue *pq) {
    if (!pq)
        return;
    free(pq->vertices);
    free(pq->priorities);
    free(pq);
}

static void pq_push(PriorityQueue *pq, GrB_Index v, double priority) {
    if (!pq || pq->size >= pq->capacity)
        return;

    GrB_Index i = pq->size++;
    pq->vertices[i] = v;
    pq->priorities[i] = priority;

    /* Heapify up */
    while (i > 0) {
        GrB_Index parent = (i - 1) / 2;
        if (pq->priorities[parent] <= pq->priorities[i])
            break;

        /* Swap */
        GrB_Index tmp_v = pq->vertices[i];
        double tmp_p = pq->priorities[i];
        pq->vertices[i] = pq->vertices[parent];
        pq->priorities[i] = pq->priorities[parent];
        pq->vertices[parent] = tmp_v;
        pq->priorities[parent] = tmp_p;
        i = parent;
    }
}

static bool pq_pop_min(PriorityQueue *pq, GrB_Index *v, double *priority) {
    if (!pq || pq->size == 0)
        return false;

    *v = pq->vertices[0];
    *priority = pq->priorities[0];

    pq->size--;
    if (pq->size > 0) {
        pq->vertices[0] = pq->vertices[pq->size];
        pq->priorities[0] = pq->priorities[pq->size];

        /* Heapify down */
        GrB_Index i = 0;
        while (2 * i + 1 < pq->size) {
            GrB_Index left = 2 * i + 1;
            GrB_Index right = 2 * i + 2;
            GrB_Index smallest = left;

            if (right < pq->size && pq->priorities[right] < pq->priorities[left]) {
                smallest = right;
            }

            if (pq->priorities[i] <= pq->priorities[smallest])
                break;

            /* Swap */
            GrB_Index tmp_v = pq->vertices[i];
            double tmp_p = pq->priorities[i];
            pq->vertices[i] = pq->vertices[smallest];
            pq->priorities[i] = pq->priorities[smallest];
            pq->vertices[smallest] = tmp_v;
            pq->priorities[smallest] = tmp_p;
            i = smallest;
        }
    }

    return true;
}

GrB_Info dijkstra_graphblas(SSSP_Result *result, LAGraph_Graph graph, GrB_Index source,
                            double delta) {
    (void)delta; /* Не используется */

    if (!result || !graph) {
        return GrB_NULL_POINTER;
    }

    sssp_result_init(result, "Dijkstra (GraphBLAS)", "Raw GraphBLAS");

    GrB_Index n;
    GrB_Matrix_nrows(&n, graph->A);
    result->vertices_processed = n;

    GrB_Vector visited = NULL;
    GrB_Monoid min_monoid = NULL;
    GrB_Semiring minplus_semiring = NULL;
    PriorityQueue *pq = NULL;
    GrB_Info info;

    info = GrB_Vector_new(&result->distances, GrB_FP64, n);
    if (info != GrB_SUCCESS)
        goto cleanup;

    info = GrB_Vector_new(&result->predecessors, GrB_UINT64, n);
    if (info != GrB_SUCCESS)
        goto cleanup;

    for (GrB_Index i = 0; i < n; i++) {
        GrB_Vector_setElement(result->distances, INF, i);
        GrB_Vector_setElement(result->predecessors, (uint64_t)-1, i);
    }
    GrB_Vector_setElement(result->distances, 0.0, source);
    GrB_Vector_setElement(result->predecessors, source, source);

    info = GrB_Vector_new(&visited, GrB_BOOL, n);
    if (info != GrB_SUCCESS)
        goto cleanup;

    for (GrB_Index i = 0; i < n; i++) {
        GrB_Vector_setElement(visited, false, i);
    }

    info = GrB_Monoid_new(&min_monoid, GrB_MIN_FP64, (double)INFINITY);
    if (info != GrB_SUCCESS)
        goto cleanup;

    info = GrB_Semiring_new(&minplus_semiring, min_monoid, GrB_PLUS_FP64);
    if (info != GrB_SUCCESS)
        goto cleanup;

    pq = pq_create(n);
    if (!pq) {
        info = GrB_OUT_OF_MEMORY;
        goto cleanup;
    }

    pq_push(pq, source, 0.0);

    result->iterations = 0;

    /* Основной цикл Dijkstra */
    while (1) {
        GrB_Index u;
        double dist;

        if (!pq_pop_min(pq, &u, &dist))
            break;

        /* Проверка на повторное посещение */
        bool is_visited;
        info = GrB_Vector_extractElement(&is_visited, visited, u);
        if (info == GrB_SUCCESS && is_visited)
            continue;

        info = GrB_Vector_setElement(visited, true, u);
        if (info != GrB_SUCCESS)
            break;

        result->iterations++;

        /* Маска для вершины u с текущим расстоянием */
        GrB_Vector u_mask = NULL;
        info = GrB_Vector_new(&u_mask, GrB_FP64, n);
        if (info != GrB_SUCCESS)
            break;

        double dist_u;
        info = GrB_Vector_extractElement(&dist_u, result->distances, u);
        if (info != GrB_SUCCESS) {
            GrB_free(&u_mask);
            break;
        }
        info = GrB_Vector_setElement(u_mask, dist_u, u);
        if (info != GrB_SUCCESS) {
            GrB_free(&u_mask);
            break;
        }

        /* Релаксация рёбер через vxm */
        GrB_Vector d_neighbors = NULL;
        info = GrB_Vector_new(&d_neighbors, GrB_FP64, n);
        if (info != GrB_SUCCESS) {
            GrB_free(&u_mask);
            break;
        }

        info = GrB_vxm(d_neighbors, NULL, NULL, minplus_semiring, u_mask, graph->A, NULL);

        if (info == GrB_SUCCESS) {
            /* Обновление расстояний и предшественников */
            for (GrB_Index v = 0; v < n; v++) {
                double old_dist, new_dist;
                GrB_Info info1 = GrB_Vector_extractElement(&old_dist, result->distances, v);
                GrB_Info info2 = GrB_Vector_extractElement(&new_dist, d_neighbors, v);

                if (info1 == GrB_SUCCESS && info2 == GrB_SUCCESS) {
                    if (new_dist < old_dist) {
                        GrB_Vector_setElement(result->distances, new_dist, v);
                        GrB_Vector_setElement(result->predecessors, (uint64_t)u, v);
                        pq_push(pq, v, new_dist);
                    }
                }
            }
        }

        GrB_free(&u_mask);
        GrB_free(&d_neighbors);
    }

    GrB_Vector_nvals(&result->reachable_vertices, result->distances);
    result->success = true;

cleanup:
    pq_free(pq);
    GrB_free(&visited);
    GrB_free(&minplus_semiring);
    GrB_free(&min_monoid);

    return info;
}
