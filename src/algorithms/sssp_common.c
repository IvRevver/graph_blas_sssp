/**
 * @file sssp_common.c
 * @brief Реализация общих функций для алгоритмов SSSP
 */

#include "sssp_common.h"

void sssp_result_init(SSSP_Result *result, const char *name, const char *source) {
    if (!result) return;
    
    memset(result, 0, sizeof(SSSP_Result));
    
    if (name) {
        strncpy(result->name, name, ALGORITHM_NAME_MAX - 1);
        result->name[ALGORITHM_NAME_MAX - 1] = '\0';
    }
    
    result->source_file = source;
    result->time_ms = 0.0;
    result->iterations = 0;
    result->success = false;
    result->distances = NULL;
    result->predecessors = NULL;
}

void sssp_result_cleanup(SSSP_Result *result) {
    if (!result) return;
    
    if (result->distances) {
        GrB_free(&result->distances);
        result->distances = NULL;
    }
    
    if (result->predecessors) {
        GrB_free(&result->predecessors);
        result->predecessors = NULL;
    }
}

SSSP_Config sssp_config_default(void) {
    SSSP_Config config;
    config.source = 0;
    config.delta = 3.0;
    config.max_iterations = 0;
    config.verbosity = 1;
    return config;
}
