/**
 * @file sssp_common.c
 * @brief Реализация общих функций для алгоритмов SSSP
 */

#include "sssp_common.h"
#include <string.h>

void sssp_result_init(SSSP_Result *result, const char *name) {
    if (!result)
        return;

    memset(result, 0, sizeof(SSSP_Result));

    if (name) {
        strncpy(result->name, name, ALGORITHM_NAME_MAX - 1);
        result->name[ALGORITHM_NAME_MAX - 1] = '\0';
    }
}

void sssp_result_cleanup(SSSP_Result *result) {
    if (!result)
        return;

    if (result->distances) {
        GrB_free(&result->distances);
        result->distances = NULL;
    }

    if (result->predecessors) {
        GrB_free(&result->predecessors);
        result->predecessors = NULL;
    }
}
