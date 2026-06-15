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
    bool has_general = (strstr(line, "general") != NULL);
    bool has_symmetric = (strstr(line, "symmetric") != NULL);
    
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
 * @param I Массив индексов строк (0-индексированные)
 * @param J Массив индексов столбцов (0-индексированные)
 * @param X Массив значений (веса рёбер)
 * @param nentries Количество элементов
 * @return GrB_SUCCESS если успешно
 */
static GrB_Info build_adjacency_matrix(GrB_Matrix A,
                                        GrB_Index *I,
                                        GrB_Index *J,
                                        double *X,
                                        GrB_Index nentries) {
    if (!A || !I || !J || !X) {
        return GrB_NULL_POINTER;
    }
    
    /* 
     * GrB_Matrix_build создаёт матрицу из кортежей (I, J, X)
     * При наличии дубликатов используется GrB_PLUS_FP64 для суммирования
     * Источник: GraphBLAS Specification v2.0, Section 5.4 [2]
     */
    GrB_Info info = GrB_Matrix_build(A, I, J, X, nentries, GrB_PLUS_FP64);
    
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
        fprintf(stderr, "❌ Файл не найден: %s\n", filename);
        return GrB_INVALID_VALUE;
    }
    
    /* Открытие файла */
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "❌ Не удалось открыть файл: %s (ошибка: %s)\n", 
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
        fprintf(stderr, "❌ Пустой файл или ошибка чтения\n");
        return GrB_INVALID_VALUE;
    }
    
    if (!validate_mm_header(line)) {
        fclose(fp);
        fprintf(stderr, "❌ Неверный формат Matrix Market\n");
        fprintf(stderr, "   Ожидалось: %%MatrixMarket matrix coordinate real ...\n");
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
        fprintf(stderr, "❌ Не удалось прочитать размеры матрицы\n");
        return GrB_INVALID_VALUE;
    }
    
    info_grb = parse_dimensions(line, &nrows, &ncols, &nentries);
    if (info_grb != GrB_SUCCESS) {
        fclose(fp);
        fprintf(stderr, "❌ Ошибка парсинга размеров: %s\n", line);
        return GrB_INVALID_VALUE;
    }
    
    /* Проверка на квадратную матрицу (требуется для графов) */
    if (nrows != ncols) {
        fclose(fp);
        fprintf(stderr, "⚠️  Матрица не квадратная (%lld × %lld)\n", 
                (long long)nrows, (long long)ncols);
        /* Продолжаем загрузку, но предупреждаем */
    }
    
    info->nverts = nrows;
    info->nedges = nentries;
    
    /* Предупреждение о большом графе */
    if (nentries > LARGE_GRAPH_THRESHOLD) {
        fprintf(stderr, "⚠️  Большой граф: %lld рёбер (может занять много памяти)\n",
                (long long)nentries);
    }
    
    /* ==========================================================================
     * Шаг 3: Выделение памяти для координат
     * ========================================================================== */
    GrB_Index *I = malloc(sizeof(GrB_Index) * nentries);
    GrB_Index *J = malloc(sizeof(GrB_Index) * nentries);
    double *X = malloc(sizeof(double) * nentries);
    
    if (!I || !J || !X) {
        free(I);
        free(J);
        free(X);
        fclose(fp);
        fprintf(stderr, "❌ Не удалось выделить память (%lld рёбер)\n",
                (long long)nentries);
        return GrB_NO_MEMORY;
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
        I[idx] = row - 1;
        J[idx] = col - 1;
        X[idx] = value;
        idx++;
        
        /* 
         * Для симметричных матриц добавляем транспонированные элементы
         * (кроме диагональных)
         */
        if (info->is_symmetric && row != col && idx < nentries) {
            I[idx] = col - 1;
            J[idx] = row - 1;
            X[idx] = value;
            idx++;
        }
    }
    
    fclose(fp);
    
    /* Проверка количества прочитанных элементов */
    if (idx == 0) {
        free(I);
        free(J);
        free(X);
        fprintf(stderr, "❌ Не удалось прочитать ни одного ребра\n");
        return GrB_INVALID_VALUE;
    }
    
    info->nedges = idx;
    info->has_weights = has_weights;
    
    /* ==========================================================================
     * Шаг 5: Создание матрицы смежности GraphBLAS
     * ========================================================================== */
    GrB_Matrix A = NULL;
    info_grb = GrB_Matrix_new(&A, GrB_FP64, nrows, ncols);
    if (info_grb != GrB_SUCCESS) {
        free(I);
        free(J);
        free(X);
        fprintf(stderr, "❌ Не удалось создать матрицу GraphBLAS\n");
        return info_grb;
    }
    
    info_grb = build_adjacency_matrix(A, I, J, X, idx);
    
    /* Освобождение временной памяти */
    free(I);
    free(J);
    free(X);
    
    if (info_grb != GrB_SUCCESS) {
        GrB_free(&A);
        fprintf(stderr, "❌ Ошибка построения матрицы: %d\n", info_grb);
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
        fprintf(stderr, "❌ Не удалось создать LAGraph_Graph\n");
        return info_grb;
    }
    
    /* ==========================================================================
     * Шаг 7: Вывод информации о загруженном графе
     * ========================================================================== */
    fprintf(stderr, "📁 Загружен граф: %s\n", info->name);
    fprintf(stderr, "   Вершин: %'lu\n", info->nverts);
    fprintf(stderr, "   Рёбер: %'lu\n", info->nedges);
    fprintf(stderr, "   Ориентированный: %s\n", info->directed ? "Да" : "Нет");
    fprintf(stderr, "   Взвешенный: %s\n", info->has_weights ? "Да" : "Нет");
    
    return GrB_SUCCESS;
}
