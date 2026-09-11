/**
 * @file sssp_common.h
 * @brief Общие структуры и функции для всех алгоритмов SSSP
 *
 */

#ifndef SSSP_COMMON_H
#define SSSP_COMMON_H

#include <stdbool.h>
#include "GraphBLAS.h"
#include "LAGraph.h"

#define ALGORITHM_NAME_MAX 64

/**
 * @brief Структура результата алгоритма SSSP
 */
typedef struct {
    char name[ALGORITHM_NAME_MAX]; /** Название алгоритма */
    double time_ms;                /** Время выполнения (мс) */
    int iterations;                /** Количество итераций */
    bool success;                  /** Флаг успеха */
    GrB_Index reachable_vertices;  /** Достижимо вершин */
    GrB_Vector distances;          /** Вектор расстояний */
    GrB_Vector predecessors;       /** Вектор предшественников */
} SSSP_Result;

/**
 * @brief Инициализация структуры результата
 */
void sssp_result_init(SSSP_Result *result, const char *name);

/**
 * @brief Очистка ресурсов результата
 */
void sssp_result_cleanup(SSSP_Result *result);

#endif /* SSSP_COMMON_H */
