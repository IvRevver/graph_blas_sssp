#include "validator.h"
#include <math.h>
#include <stdio.h>
#include <float.h>


static bool is_infinite(double value) {
    return isinf(value) || (value >= DBL_MAX / 2);
}


static bool float_equal(double a, double b, double epsilon) {
    /* Оба бесконечные - считаются равными */
    if (is_infinite(a) && is_infinite(b)) {
        return true;
    }
    
    /* Одно бесконечное, другое нет - не равны */
    if (is_infinite(a) || is_infinite(b)) {
        return false;
    }
    
    /* Сравнение с погрешностью */
    return fabs(a - b) < epsilon;
}


bool sssp_validate_source_distance(GrB_Vector distances, GrB_Index source) {
    /* Проверка входных параметров */
    if (!distances) {
        return false;
    }
    
    /* Извлечение расстояния до источника */
    double source_dist;
    GrB_Info info = GrB_Vector_extractElement(&source_dist, distances, source);
    
    /* Проверка успешности извлечения */
    if (info != GrB_SUCCESS) {
        return false;
    }
    
    /* Расстояние до источника должно быть ровно 0 */
    /* Источник: Cormen et al., Section 24.1 [1] */
    return (source_dist == 0.0);
}

bool sssp_validate_non_negative(GrB_Vector distances) {
    if (!distances) {
        return false;
    }
    
    /* Получение размера вектора */
    GrB_Index n;
    GrB_Info info = GrB_Vector_size(&n, distances);
    if (info != GrB_SUCCESS) {
        return false;
    }
    
    /* Проверка каждого элемента */
    for (GrB_Index i = 0; i < n; i++) {
        double dist;
        info = GrB_Vector_extractElement(&dist, distances, i);
        
        if (info == GrB_SUCCESS) {
            /* 
             * Расстояние должно быть ≥ 0 или ∞
             * Отрицательное конечное значение = ошибка
             * Источник: Bellman-Ford корректность [2]
             */
            if (dist < 0.0 && !is_infinite(dist)) {
                return false;
            }
        }
        /* GrB_NO_VALUE означает недостижимую вершину - это OK */
    }
    
    return true;
}

bool sssp_validate_distances(GrB_Vector v1, GrB_Vector v2) {
    /* Проверка входных параметров */
    if (!v1 || !v2) {
        return false;
    }
    
    /* Получение размеров векторов */
    GrB_Index n1, n2;
    GrB_Info info1 = GrB_Vector_size(&n1, v1);
    GrB_Info info2 = GrB_Vector_size(&n2, v2);
    
    if (info1 != GrB_SUCCESS || info2 != GrB_SUCCESS) {
        return false;
    }
    
    /* Векторы должны быть одинакового размера */
    if (n1 != n2) {
        return false;
    }
    
    /* Попарное сравнение элементов */
    for (GrB_Index i = 0; i < n1; i++) {
        double d1, d2;
        GrB_Info info_d1 = GrB_Vector_extractElement(&d1, v1, i);
        GrB_Info info_d2 = GrB_Vector_extractElement(&d2, v2, i);
        
        /* 
         * Оба элемента должны либо существовать, либо отсутствовать
         */
        if (info_d1 != info_d2) {
            return false;
        }
        
        /* Если оба существуют - сравниваем значения */
        if (info_d1 == GrB_SUCCESS) {
            if (!float_equal(d1, d2, VALIDATOR_EPSILON)) {
                return false;
            }
        }
    }
    
    return true;
}

bool sssp_validate_result(SSSP_Result *result, GrB_Index source) {
    /* Проверка указателя */
    if (!result) {
        fprintf(stderr, "  ❌ result == NULL\n");
        return false;
    }
    
    /* Проверка флага успеха */
    if (!result->success) {
        fprintf(stderr, "  ❌ result->success == false\n");
        return false;
    }
    
    /* Проверка вектора расстояний */
    if (!result->distances) {
        fprintf(stderr, "  ❌ result->distances == NULL\n");
        return false;
    }
    
    bool valid = true;
    
    /* Проверка 1: расстояние до источника */
    if (!sssp_validate_source_distance(result->distances, source)) {
        fprintf(stderr, "  ❌ Расстояние до источника != 0\n");
        valid = false;
    }
    
    /* Проверка 2: неотрицательность */
    if (!sssp_validate_non_negative(result->distances)) {
        fprintf(stderr, "  ❌ Обнаружены отрицательные расстояния\n");
        valid = false;
    }
    
    /* Проверка 3: есть достижимые вершины */
    if (result->reachable_vertices == 0) {
        fprintf(stderr, "  ❌ Нет достижимых вершин (возможно ошибка)\n");
        valid = false;
    }
    
    return valid;
}

void sssp_print_validation_report(SSSP_Result *result, 
                                   GrB_Index source, 
                                   FILE *stream) {
    if (!result || !stream) {
        return;
    }
    
    fprintf(stream, "\n");
    fprintf(stream, "════════════════════════════════════════════════════\n");
    fprintf(stream, "Отчёт валидации для: %s\n", result->name);
    fprintf(stream, "════════════════════════════════════════════════════\n");
    
    /* Статус алгоритма */
    fprintf(stream, "  Статус выполнения: %s\n", 
            result->success ? "✅ Успех" : "❌ Ошибка");
    
    if (!result->success) {
        fprintf(stream, "\n  ⚠️  Алгоритм не выполнился успешно, проверка пропущена\n");
        return;
    }
    
    /* Статистика */
    fprintf(stream, "  Время выполнения: %.2f мс\n", result->time_ms);
    fprintf(stream, "  Итераций: %.0f\n", result->iterations);
    fprintf(stream, "  Достижимо вершин: %lu\n", result->reachable_vertices);
    
    /* Проверки */
    fprintf(stream, "\n  Проверки:\n");
    
    bool check1 = sssp_validate_source_distance(result->distances, source);
    fprintf(stream, "    dist[source] == 0: %s\n", check1 ? "✅" : "❌");
    
    bool check2 = sssp_validate_non_negative(result->distances);
    fprintf(stream, "    Все dist >= 0: %s\n", check2 ? "✅" : "❌");
    
    bool check3 = (result->reachable_vertices > 0);
    fprintf(stream, "    reachable_vertices > 0: %s\n", check3 ? "✅" : "❌");
    
    /* Итог */
    fprintf(stream, "\n  Итог: %s\n", 
            (check1 && check2 && check3) ? "✅ Все проверки пройдены" 
                                          : "❌ Проверки не пройдены");
}