/**
 * @file algebraic_bf_graphblas.h
 * @brief Algebraic Bellman-Ford через Raw GraphBLAS
 *
 * Алгоритм: Algebraic Bellman-Ford для SSSP
 */

#ifndef ALGEBRAIC_BF_GRAPHBLAS_H
#define ALGEBRAIC_BF_GRAPHBLAS_H

#include "sssp_common.h"

/**
 * @brief Запуск Algebraic Bellman-Ford через GraphBLAS
 *
 * @param result Структура результата
 * @param graph Граф LAGraph
 * @param source Исходная вершина
 * @param delta Не используется (для совместимости интерфейса)
 * @return GrB_SUCCESS если успешно
 */
GrB_Info algebraic_bf_graphblas(SSSP_Result *result, LAGraph_Graph graph, GrB_Index source,
                                double delta);

#endif /* ALGEBRAIC_BF_GRAPHBLAS_H */
