/**
 * @file graph_loader.c
 * @brief Реализация загрузки графов из формата Matrix Market
 * 
 * Формат файла .mtx:
 * Строка 1:  %%MatrixMarket matrix coordinate real general
 * Строка 2:  nrows ncols nentries
 * Строка 3+: row col value (1-индексированные)
 * 
 * Источники:
 * - Matrix Market Format [1]
 * - LAGraph MMRead implementation [3]
 * - GraphBLAS Specification [2]
 */

#include "graph_loader.h"
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

/**
 * @brief Максимальная длина строки при чтении файла
 */
#define MAX_LINE_LENGTH 1024

/**
 * @brief Порог для предупреждения о большом графе
 */
#define LARGE_GRAPH_THRESHOLD 1000000

/* ==========================================================================
 *                           Вспомогательные функции
 * ========================================================================== */

bool graph_file_exists(const char *filename) {
    if (!filename) return false;
    
    struct stat st;
    return (stat(filename, &st) == 0 && S_ISREG(st.st_mode));
}

GrB_Info graph_extract_name(const char *filename, char *name, size_t name_size) {
    if (!filename || !name || name_size == 0) {
        return GrB_NULL_POINTER;
    }
    
    /* Найти последнее '/' или '\' */
    const char *basename = strrchr(filename, '/');
    if (!basename) {
        basename = strrchr(filename, '\\');
    }
    basename = basename ? basename + 1 : filename;
    
    /* Копировать имя */
    strncpy(name, basename, name_size - 1);
    name[name_size - 1] = '\0';
    
    /* Удалить расширение .mtx */
    char *dot = strrchr(name, '.');
    if (dot && strcmp(dot, ".mtx") == 0) {
        *dot = '\0';
    }
    
    return GrB_SUCCESS;
}

void graph_info_cleanup(GraphInfo *info) {
    if (!info) return;
    /* В текущей реализации динамическая память не выделяется */
    memset(info, 0, sizeof(GraphInfo));
}

/* ==========================================================================
 *                      Парсинг заголовка Matrix Market
 * ========================================================================== */

/**
 * @brief Проверить строку заголовка Matrix Market
 * 
 * Ожидаемый формат: %%MatrixMarket matrix coordinate real general
 * 
 * @return true если заголовок корректен
 */
static bool validate_mm_header(const char *line) {
    if (!line) return false;
    
    /* Проверка на наличие ключевых слов */
    bool has_matrix_market = (strstr(line, "%%MatrixMarket") != NULL);
    bool has_matrix = (strstr(line, "matrix") != NULL);
    bool has_coordinate = (strstr(line, "coordinate") != NULL);
    
    /* Поддерживаемые типы данных */
    bool has_real = (strstr(line, "real") != NULL);
    bool has_pattern = (strstr(line, "pattern") != NULL);
    bool has_integer = (strstr(line, "integer") != NULL);
    
    /* Структура матрицы */
    
    if (!has_matrix_market || !has_matrix || !has_coordinate) {
        return false;
    }
    
    /* Хотя бы один тип данных должен быть указан */
    if (!has_real && !has_pattern && !has_integer) {
        return false;
    }
    
    return true;
}

/**
 * @brief Пропустить комментарии в файле Matrix Market
 * 
 * Комментарии начинаются с символа '%'
 * 
 * @param fp Файловый указатель
 * @param line Буфер для чтения
 * @return GrB_SUCCESS если успешно
 */
static GrB_Info skip_comments(FILE *fp, char *line) {
    if (!fp || !line) {
        return GrB_NULL_POINTER;
    }
    
    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        /* Пропустить пустые строки */
        if (line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        
        /* Если не комментарий - вернуть строку */
        if (line[0] != '%') {
            return GrB_SUCCESS;
        }
    }
    
    return GrB_NO_VALUE;  /* Конец файла */
}

/* ==========================================================================
 *                      Чтение данных графа
 * ========================================================================== */

/**
 * @brief Прочитать размеры матрицы из файла
 * 
 * Формат строки: nrows ncols nentries
 * 
 * @param line Строка с размерами
 * @param nrows Количество строк (выход)
 * @param ncols Количество столбцов (выход)
 * @param nentries Количество элементов (выход)
 * @return GrB_SUCCESS если успешно
 */
static GrB_Info parse_dimensions(const char *line, 
                                  GrB_Index *nrows, 
                                  GrB_Index *ncols, 
                                  GrB_Index *nentries) {
    if (!line || !nrows || !ncols || !nentries) {
        return GrB_NULL_POINTER;
    }
    
    long long nr, nc, ne;
    int parsed = sscanf(line, "%lld %lld %lld", &nr, &nc, &ne);
    
    if (parsed != 3) {
        return GrB_INVALID_VALUE;
    }
    
    if (nr <= 0 || nc <= 0 || ne <= 0) {
        return GrB_INVALID_VALUE;
    }
    
    /* Проверка на переполнение */
    if (nr > (long long)INT64_MAX || nc > (long long)INT64_MAX) {
        return GrB_INVALID_VALUE;
    }
    
    *nrows = (GrB_Index)nr;
    *ncols = (GrB_Index)nc;
    *nentries = (GrB_Index)ne;
    
    return GrB_SUCCESS;
}

/**
 * @brief Построить матрицу смежности из координат
 * 
 * @param A Матрица для заполнения
 * @param rows Массив индексов строк (0-индексированные)
 * @param cols Массив индексов столбцов (0-индексированные)
 * @param vals Массив значений (веса рёбер)
 * @param nentries Количество элементов
 * @return GrB_SUCCESS если успешно
 */
static GrB_Info build_adjacency_matrix(GrB_Matrix A,
                                        GrB_Index *rows,
                                        GrB_Index *cols,
                                        double *vals,
                                        GrB_Index nentries) {
    if (!A || !rows || !cols || !vals) {
        return GrB_NULL_POINTER;
    }
    
    /* 
     * GrB_Matrix_build создаёт матрицу из кортежей (I, J, X)
     * При наличии дубликатов используется GrB_PLUS_FP64 для суммирования
     * Источник: GraphBLAS Specification v2.0, Section 5.4 [2]
     */
    GrB_Info info = GrB_Matrix_build(A, rows, cols, vals, nentries, GrB_PLUS_FP64);
    
    return info;
}

/* ==========================================================================
 *                      Основная функция загрузки
 * ========================================================================== */

GrB_Info graph_load(LAGraph_Graph *graph, const char *filename, GraphInfo *info) {
    /* Проверка входных параметров */
    if (!graph || !filename || !info) {
        return GrB_NULL_POINTER;
    }
    
    /* Инициализация структуры info */
    memset(info, 0, sizeof(GraphInfo));
    strncpy(info->path, filename, MAX_FILENAME - 1);
    
    /* Извлечение имени графа */
    GrB_Info info_grb = graph_extract_name(filename, info->name, MAX_GRAPH_NAME);
    if (info_grb != GrB_SUCCESS) {
        return info_grb;
    }
    
    /* Проверка существования файла */
    if (!graph_file_exists(filename)) {
        fprintf(stderr, "[FAIL] File not found: %s\n", filename);
        return GrB_INVALID_VALUE;
    }
    
    /* Открытие файла */
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[FAIL] Could not open file: %s (error: %s)\n", 
                filename, strerror(errno));
        return GrB_INVALID_VALUE;
    }
    
    char line[MAX_LINE_LENGTH];
    GrB_Index nrows, ncols, nentries;
    
    /* ==========================================================================
     * Шаг 1: Чтение и проверка заголовка
     * ========================================================================== */
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        fprintf(stderr, "[FAIL] Empty file or read error\n");
        return GrB_INVALID_VALUE;
    }
    
    if (!validate_mm_header(line)) {
        fclose(fp);
        fprintf(stderr, "[FAIL] Invalid Matrix Market format\n");
        fprintf(stderr, "       Expected: %%MatrixMarket matrix coordinate real ...\n");
        return GrB_INVALID_VALUE;
    }
    
    /* Проверка на симметричную матрицу */
    info->is_symmetric = (strstr(line, "symmetric") != NULL);
    info->directed = !info->is_symmetric;
    
    /* ==========================================================================
     * Шаг 2: Пропуск комментариев и чтение размеров
     * ========================================================================== */
    info_grb = skip_comments(fp, line);
    if (info_grb != GrB_SUCCESS) {
        fclose(fp);
        fprintf(stderr, "[FAIL] Could not read matrix dimensions\n");
        return GrB_INVALID_VALUE;
    }
    
    info_grb = parse_dimensions(line, &nrows, &ncols, &nentries);
    if (info_grb != GrB_SUCCESS) {
        fclose(fp);
        fprintf(stderr, "[FAIL] Could not parse dimensions: %s\n", line);
        return GrB_INVALID_VALUE;
    }
    
    /* Проверка на квадратную матрицу (требуется для графов) */
    if (nrows != ncols) {
        fclose(fp);
        fprintf(stderr, "[!] Matrix is not square (%lld x %lld)\n", 
                (long long)nrows, (long long)ncols);
        /* Продолжаем загрузку, но предупреждаем */
    }
    
    info->nverts = nrows;
    info->nedges = nentries;
    
    /* Предупреждение о большом графе */
    if (nentries > LARGE_GRAPH_THRESHOLD) {
        fprintf(stderr, "[!] Large graph: %lld edges (may use significant memory)\n",
                (long long)nentries);
    }
    
    /* ==========================================================================
     * Шаг 3: Выделение памяти для координат
     * ========================================================================== */
    GrB_Index *row_idx = malloc(sizeof(GrB_Index) * nentries);
    GrB_Index *col_idx = malloc(sizeof(GrB_Index) * nentries);
    double *weights = malloc(sizeof(double) * nentries);
    
    if (!row_idx || !col_idx || !weights) {
        free(row_idx);
        free(col_idx);
        free(weights);
        fclose(fp);
        fprintf(stderr, "[FAIL] Could not allocate memory (%lld edges)\n",
                (long long)nentries);
        return GrB_OUT_OF_MEMORY;
    }
    
    /* ==========================================================================
     * Шаг 4: Чтение элементов матрицы
     * ========================================================================== */
    GrB_Index idx = 0;
    bool has_weights = false;
    
    while (fgets(line, sizeof(line), fp) && idx < nentries) {
        /* Пропустить комментарии */
        if (line[0] == '%') {
            continue;
        }
        
        /* Пропустить пустые строки */
        if (line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        
        long long row_l, col_l;
        double value = 1.0;
        
        int parsed = sscanf(line, "%lld %lld %lf", &row_l, &col_l, &value);
        GrB_Index row = (GrB_Index)row_l;
        GrB_Index col = (GrB_Index)col_l;
        
        if (parsed < 2) {
            continue;  /* Пропустить некорректную строку */
        }
        
        if (parsed == 3) {
            has_weights = true;
        }
        
        /* 
         * Конвертация 1-индексации Matrix Market в 0-индексацию GraphBLAS
         * Источник: Matrix Market Format Specification [1]
         */
        row_idx[idx] = row - 1;
        col_idx[idx] = col - 1;
        weights[idx] = value;
        idx++;
        
        /* 
         * Для симметричных матриц добавляем транспонированные элементы
         * (кроме диагональных)
         */
        if (info->is_symmetric && row != col && idx < nentries) {
            row_idx[idx] = col - 1;
            col_idx[idx] = row - 1;
            weights[idx] = value;
            idx++;
        }
    }
    
    fclose(fp);
    
    /* Проверка количества прочитанных элементов */
    if (idx == 0) {
        free(row_idx);
        free(col_idx);
        free(weights);
        fprintf(stderr, "[FAIL] Could not read any edges\n");
        return GrB_INVALID_VALUE;
    }
    
    info->nedges = idx;
    info->has_weights = has_weights;
    
    /* Проверка на отрицательные веса */
    info->has_negative_weights = false;
    for (GrB_Index i = 0; i < idx; i++) {
        if (weights[i] < 0) {
            info->has_negative_weights = true;
            break;
        }
    }

    /* ==========================================================================
     * Шаг 5: Создание матрицы смежности GraphBLAS
     * ========================================================================== */
    GrB_Matrix A = NULL;
    info_grb = GrB_Matrix_new(&A, GrB_FP64, nrows, ncols);
    if (info_grb != GrB_SUCCESS) {
        free(row_idx);
        free(col_idx);
        free(weights);
        fprintf(stderr, "[FAIL] Could not create GraphBLAS matrix\n");
        return info_grb;
    }
    
    info_grb = build_adjacency_matrix(A, row_idx, col_idx, weights, idx);
    
    /* Освобождение временной памяти */
    free(row_idx);
    free(col_idx);
    free(weights);
    
    if (info_grb != GrB_SUCCESS) {
        GrB_free(&A);
        fprintf(stderr, "[FAIL] Matrix build error: %d\n", info_grb);
        return info_grb;
    }
    
    /* ==========================================================================
     * Шаг 6: Создание LAGraph графа
     * ========================================================================== */
    char msg[LAGRAPH_MSG_LEN];
    LAGraph_Kind kind = info->is_symmetric ? LAGraph_ADJACENCY_UNDIRECTED : LAGraph_ADJACENCY_DIRECTED;
    info_grb = LAGraph_New(graph, &A, kind, msg);
    if (info_grb != GrB_SUCCESS) {
        GrB_free(&A);
        fprintf(stderr, "[FAIL] Could not create LAGraph_Graph\n");
        return info_grb;
    }
    
    /* ==========================================================================
     * Шаг 7: Вывод информации о загруженном графе
     * ========================================================================== */
    fprintf(stdout, "[LOAD] Graph: %s\n", info->name);
    fprintf(stdout, "       Vertices: %llu\n", (unsigned long long)info->nverts);
    fprintf(stdout, "       Edges: %llu\n", (unsigned long long)info->nedges);
    fprintf(stdout, "       Directed: %s\n", info->directed ? "Yes" : "No");
    fprintf(stdout, "       Weighted: %s\n", info->has_weights ? "Yes" : "No");
    if (info->has_negative_weights) {
        fprintf(stdout, "       Negative weights: Yes\n");
    }
    
    return GrB_SUCCESS;
}
