/**
 * @file dijkstra_graphblas.h
 * @brief Dijkstra алгоритм через Raw GraphBLAS
 *
 * Алгоритм: Dijkstra с приоритетной очередью
 * Ограничение: Только для неотрицательных весов
 */

#ifndef DIJKSTRA_GRAPHBLAS_H
#define DIJKSTRA_GRAPHBLAS_H

#include "sssp_common.h"

/**
 * @brief Запуск Dijkstra через GraphBLAS
 *
 * @param result Структура результата
 * @param graph Граф LAGraph
 * @param source Исходная вершина
 * @param delta Не используется (для совместимости интерфейса)
 * @return GrB_SUCCESS если успешно
 */

GrB_Info dijkstra_graphblas(SSSP_Result *result, LAGraph_Graph graph, GrB_Index source,
                            double delta);

#endif /* DIJKSTRA_GRAPHBLAS_H */
