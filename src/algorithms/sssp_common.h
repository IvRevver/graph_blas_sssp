/**
 * @file sssp_common.h
 * @brief Общие структуры и функции для всех алгоритмов SSSP
 * 
 */

#ifndef SSSP_COMMON_H
#define SSSP_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "GraphBLAS.h"
#include "LAGraph.h"

#define INF INFINITY
#define ALGORITHM_NAME_MAX 64

/**
 * @brief Структура результата алгоритма SSSP
 */
typedef struct {
    char name[ALGORITHM_NAME_MAX];      /** Название алгоритма */
    double time_ms;                      /** Время выполнения (мс) */
    int iterations;                      /** Количество итераций */
    bool success;                        /** Флаг успеха */
    GrB_Index vertices_processed;        /** Обработано вершин */
    GrB_Index reachable_vertices;        /** Достижимо вершин */
    GrB_Vector distances;                /** Вектор расстояний */
    GrB_Vector predecessors;             /** Вектор предшественников */
    const char *source_file;             /** Источник реализации */
} SSSP_Result;

/**
 * @brief Конфигурация алгоритма
 */
typedef struct {
    GrB_Index source;                    /** Исходная вершина */
    double delta;                        /** Параметр delta для Delta-Stepping */
    int max_iterations;                  /** Максимум итераций */
    int verbosity;                       /** Уровень детализации */
} SSSP_Config;

/**
 * @brief Инициализация структуры результата
 */
void sssp_result_init(SSSP_Result *result, const char *name, const char *source);

/**
 * @brief Очистка ресурсов результата
 */
void sssp_result_cleanup(SSSP_Result *result);

/**
 * @brief Конфигурация по умолчанию
 */
SSSP_Config sssp_config_default(void);

#endif /* SSSP_COMMON_H */
