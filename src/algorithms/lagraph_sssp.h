/**
 * @file lagraph_sssp.h
 * @brief LAGraph SSSP (Delta-Stepping) через LAGr_SingleSourceShortestPath
 *
 */

#ifndef LAGRAPH_SSSP_H
#define LAGRAPH_SSSP_H

#include "sssp_common.h"

/**
 * @brief Запуск LAGraph SSSP (Delta-Stepping)
 *
 * @param result Структура результата
 * @param graph Граф LAGraph
 * @param source Исходная вершина
 * @param delta Параметр шага для Delta-Stepping
 * @return GrB_SUCCESS если успешно
 */

GrB_Info lagraph_sssp(SSSP_Result *result, LAGraph_Graph graph, GrB_Index source, double delta);

#endif /* LAGRAPH_SSSP_H */
