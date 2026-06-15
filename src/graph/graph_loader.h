/**
 * @file graph_loader.h
 * @brief Загрузка графов из файлов формата Matrix Market
 * 
 * Формат Matrix Market:
 * - Разработан NIST (National Institute of Standards and Technology)
 * - Поддерживает разреженные матрицы в координатном формате
 * - Использует 1-индексацию (требуется конвертация для GraphBLAS)
 * 
 * Источники:
 * - Matrix Market Format Specification [1]
 * - GraphBLAS Specification v2.0 [2]
 */

#ifndef GRAPH_LOADER_H
#define GRAPH_LOADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GraphBLAS.h"
#include "LAGraph.h"

/**
 * @brief Максимальная длина пути к файлу
 */
#define MAX_FILENAME 512

/**
 * @brief Максимальная длина имени графа
 */
#define MAX_GRAPH_NAME 256

/**
 * @brief Информация о загруженном графе
 */
typedef struct {
    char name[MAX_GRAPH_NAME];      /** Имя графа (из имени файла) */
    char path[MAX_FILENAME];        /** Полный путь к файлу */
    GrB_Index nverts;               /** Количество вершин */
    GrB_Index nedges;               /** Количество рёбер */
    bool directed;                  /** true = ориентированный, false = неориентированный */
    bool has_weights;               /** true = взвешенный граф */
    bool is_symmetric;              /** true = симметричная матрица */
} GraphInfo;

/**
 * @brief Загрузить граф из файла Matrix Market
 * 
 * @param graph Указатель на граф LAGraph (выход)
 * @param filename Путь к файлу .mtx
 * @param info Структура для информации о графе (выход)
 * @return GrB_SUCCESS если успешно, иначе код ошибки GraphBLAS
 * 
 * @note Файл должен быть в формате Matrix Market coordinate real general
 * @note 1-индексация конвертируется в 0-индексацию автоматически
 */
GrB_Info graph_load(LAGraph_Graph *graph, const char *filename, GraphInfo *info);

/**
 * @brief Извлечь имя графа из пути к файлу
 * 
 * @param filename Полный путь к файлу
 * @param name Буфер для имени (должен быть выделен)
 * @param name_size Размер буфера
 * @return GrB_SUCCESS если успешно
 */
GrB_Info graph_extract_name(const char *filename, char *name, size_t name_size);

/**
 * @brief Проверить существование файла
 * 
 * @param filename Путь к файлу
 * @return true если файл существует и доступен для чтения
 */
bool graph_file_exists(const char *filename);

/**
 * @brief Очистить ресурсы GraphInfo
 * 
 * @param info Структура для очистки
 */
void graph_info_cleanup(GraphInfo *info);

#endif /* GRAPH_LOADER_H */
