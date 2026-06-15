#ifndef VALIDATOR_H
#define VALIDATOR_H

#include <stdbool.h>
#include "GraphBLAS.h"
#include "algorithms/sssp_common.h"

#define VALIDATOR_EPSILON 1e-6

bool sssp_validate_source_distance(GrB_Vector distances, GrB_Index source);

bool sssp_validate_non_negative(GrB_Vector distances);

bool sssp_validate_distances(GrB_Vector v1, GrB_Vector v2);

bool sssp_validate_result(SSSP_Result *result, GrB_Index source);

void sssp_print_validation_report(SSSP_Result *result, 
                                   GrB_Index source, 
                                   FILE *stream);

#endif /* VALIDATOR_H */
