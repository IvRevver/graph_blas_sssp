# ==============================================================================
# SSSP GraphBLAS Benchmark - Makefile
# ==============================================================================
# 
# Описание:
#   Этот Makefile управляет сборкой, тестированием и запуском бенчмарка
#   алгоритмов SSSP с использованием GraphBLAS и LAGraph.
#
# Требования:
#   - GCC или совместимый C компилятор с поддержкой C11
#   - SuiteSparse GraphBLAS [1]
#   - LAGraph [2]
#   - OpenMP (опционально, для параллелизма)
#
# Основные цели:
#   make          - Сборка проекта
#   make test     - Запуск юнит-тестов
#   make run      - Запуск бенчмарка на тестовом графе
#   make clean    - Очистка скомпилированных файлов
#   make install  - Установка в систему (требует sudo)
#
# Источники:
#   [1] GNU Make Manual [3]
#   [2] GraphBLAS Specification v2.0 [4]
#   [3] SuiteSparse GraphBLAS Repository [5]
#   [4] LAGraph Repository [6]
#
# ==============================================================================

# ==============================================================================
# Компилятор и флаги
# ==============================================================================

# Компилятор C
# Источник: GCC Documentation [7]
CC = gcc

# Стандарт C: C11 (требуется для GraphBLAS)
# Источник: C11 Standard, Section 6.10.8 [8]
CSTD = -std=c11

# Флаги компиляции
# -O3: Максимальная оптимизация (важно для бенчмарков)
# -Wall -Wextra: Предупреждения компилятора
# -fopenmp: Поддержка OpenMP (если доступен)
# Источник: GCC Optimization Options [9]
CFLAGS = $(CSTD) -O3 -Wall -Wextra -fopenmp

# Флаги для отладочной сборки
# -g: Отладочная информация
# -DDEBUG: Макрос для условной компиляции
DEBUG_CFLAGS = $(CSTD) -g -Wall -Wextra -fopenmp -DDEBUG

# Флаги для профилирования
# -pg: Вставка кода для gprof
# Источник: GNU gprof Documentation [10]
PROFILE_CFLAGS = $(CSTD) -O3 -Wall -Wextra -fopenmp -pg

# Пути для заголовочных файлов
# Isrc: Исходный код проекта
# /usr/local/include: Системные заголовки (GraphBLAS, LAGraph)
INCLUDES = -Isrc -I/usr/local/include

# Флаги линковки
# -lgraphblas: SuiteSparse GraphBLAS
# -llagraph: LAGraph
# -lm: Математическая библиотека (требуется для math.h функций)
# -fopenmp: OpenMP для параллелизма
# Источник: GraphBLAS Installation Guide [5]
LDFLAGS = -lgraphblas -llagraph -lm -fopenmp

# Пути для библиотек
# /usr/local/lib: Стандартный путь для установленных библиотек
LIBPATHS = -L/usr/local/lib

# ==============================================================================
# Директории
# ==============================================================================

# Корневая директория исходного кода
SRCDIR = src

# Директория тестов
TESTDIR = tests

# Директория для скомпилированных объектов
OBJDIR = obj

# Директория для исполняемых файлов
BINDIR = bin

# Директория для результатов
RESULTSDIR = results

# Директория для графов
GRAPHDIR = graphs

# ==============================================================================
# Исходные файлы
# ==============================================================================

# Основные исходные файлы проекта
MAIN_SOURCES = $(SRCDIR)/main.c \
               $(SRCDIR)/algorithms/sssp_common.c \
               $(SRCDIR)/algorithms/lagraph_sssp.c \
               $(SRCDIR)/algorithms/algebraic_bf_graphblas.c \
               $(SRCDIR)/algorithms/dijkstra_graphblas.c \
               $(SRCDIR)/graph/graph_loader.c \
               $(SRCDIR)/utils/timer.c \
               $(SRCDIR)/utils/validator.c

# Исходные файлы тестов
TEST_SOURCES = $(TESTDIR)/test_all.c \
               $(SRCDIR)/algorithms/sssp_common.c \
               $(SRCDIR)/algorithms/lagraph_sssp.c \
               $(SRCDIR)/algorithms/algebraic_bf_graphblas.c \
               $(SRCDIR)/algorithms/dijkstra_graphblas.c \
               $(SRCDIR)/graph/graph_loader.c \
               $(SRCDIR)/utils/timer.c \
               $(SRCDIR)/utils/validator.c

# Заголовочные файлы (для зависимостей)
HEADERS = $(SRCDIR)/algorithms/sssp_common.h \
          $(SRCDIR)/algorithms/lagraph_sssp.h \
          $(SRCDIR)/algorithms/algebraic_bf_graphblas.h \
          $(SRCDIR)/algorithms/dijkstra_graphblas.h \
          $(SRCDIR)/graph/graph_loader.h \
          $(SRCDIR)/utils/timer.h \
          $(SRCDIR)/utils/validator.h

# Объектные файлы для основной сборки
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(MAIN_SOURCES))

# Объектные файлы для тестов
TEST_OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(filter-out $(SRCDIR)/main.c,$(TEST_SOURCES))) \
               $(OBJDIR)/test_all.o

# ==============================================================================
# Целевые файлы
# ==============================================================================

# Основной исполняемый файл
TARGET = $(BINDIR)/sssp_benchmark

# Исполняемый файл тестов
TEST_TARGET = $(BINDIR)/test_sssp

# ==============================================================================
# Основные цели
# ==============================================================================

# Цель по умолчанию
.PHONY: all
all: dirs $(TARGET)

# Создание необходимых директорий
.PHONY: dirs
dirs:
	@mkdir -p $(OBJDIR)/algorithms
	@mkdir -p $(OBJDIR)/graph
	@mkdir -p $(OBJDIR)/utils
	@mkdir -p $(BINDIR)
	@mkdir -p $(RESULTSDIR)

# ==============================================================================
# Сборка основного проекта
# ==============================================================================

$(TARGET): $(OBJECTS)
	@echo "🔗 Линковка $(TARGET)..."
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBPATHS)
	@echo "✅ Сборка завершена: $(TARGET)"

# Компиляция основных исходных файлов
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@echo "📝 Компиляция $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Специальное правило для test_all.c
$(OBJDIR)/test_all.o: $(TESTDIR)/test_all.c $(HEADERS)
	@echo "📝 Компиляция $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ==============================================================================
# Сборка тестов
# ==============================================================================

.PHONY: test
test: dirs $(TEST_TARGET)
	@echo ""
	@echo "🧪 Запуск тестов..."
	@echo "════════════════════════════════════════════════════"
	@./$(TEST_TARGET)
	@echo "════════════════════════════════════════════════════"

$(TEST_TARGET): $(TEST_OBJECTS)
	@echo "🔗 Линковка $(TEST_TARGET)..."
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJECTS) $(LDFLAGS) $(LIBPATHS)
	@echo "✅ Тесты собраны: $(TEST_TARGET)"

# ==============================================================================
# Запуск бенчмарка
# ==============================================================================

.PHONY: run
run: $(TARGET)
	@echo ""
	@echo "🚀 Запуск бенчмарка на graphs/test.mtx..."
	@echo "════════════════════════════════════════════════════"
	@./$(TARGET) $(GRAPHDIR)/test.mtx 0 3.0
	@echo "════════════════════════════════════════════════════"

.PHONY: run-graph
run-graph: $(TARGET)
	@if [ -z "$(GRAPH)" ]; then \
		echo "❌ Ошибка: укажите граф через GRAPH=путь/к/файлу.mtx"; \
		echo "   Пример: make run-graph GRAPH=graphs/lesmis.mtx"; \
		exit 1; \
	fi
	@echo "🚀 Запуск бенчмарка на $(GRAPH)..."
	@./$(TARGET) $(GRAPH) $(SOURCE) $(DELTA)

# Параметры по умолчанию для run-graph
SOURCE ?= 0
DELTA ?= 3.0

# ==============================================================================
# Отладочная сборка
# ==============================================================================

.PHONY: debug
debug: CFLAGS = $(DEBUG_CFLAGS)
debug: clean dirs $(TARGET)
	@echo "✅ Отладочная сборка завершена"

# ==============================================================================
# Профилирование
# ==============================================================================

.PHONY: profile
profile: CFLAGS = $(PROFILE_CFLAGS)
profile: LDFLAGS += -pg
profile: clean dirs $(TARGET)
	@echo "✅ Профилировочная сборка завершена"
	@echo "   Запустите: make run"
	@echo "   Затем: gprof $(TARGET) gmon.out > profile.txt"

# ==============================================================================
# Очистка
# ==============================================================================

.PHONY: clean
clean:
	@echo "🧹 Очистка скомпилированных файлов..."
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)
	rm -f gmon.out
	rm -f profile.txt
	@echo "✅ Очистка завершена"

.PHONY: distclean
distclean: clean
	@echo "🧹 Удаление результатов бенчмарков..."
	rm -rf $(RESULTSDIR)/*
	@echo "✅ Полная очистка завершена"

# ==============================================================================
# Установка
# ==============================================================================

.PHONY: install
install: $(TARGET)
	@echo "📦 Установка $(TARGET) в /usr/local/bin..."
	sudo install -m 755 $(TARGET) /usr/local/bin/
	@echo "✅ Установка завершена"

.PHONY: uninstall
uninstall:
	@echo "🗑️  Удаление из /usr/local/bin..."
	sudo rm -f /usr/local/bin/sssp_benchmark
	@echo "✅ Удаление завершено"

# ==============================================================================
# Документация
# ==============================================================================

.PHONY: docs
docs:
	@echo "📚 Генерация документации (требуется Doxygen)..."
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen Doxyfile 2>/dev/null || echo "⚠️  Doxyfile не найден"; \
	else \
		echo "❌ Doxygen не установлен. Установите: sudo apt-get install doxygen"; \
	fi

# ==============================================================================
# Проверка зависимостей
# ==============================================================================

.PHONY: check-deps
check-deps:
	@echo "🔍 Проверка зависимостей..."
	@echo ""
	@# Проверка компилятора
	@if command -v $(CC) >/dev/null 2>&1; then \
		echo "✅ Компилятор: $$($(CC) --version | head -1)"; \
	else \
		echo "❌ Компилятор $(CC) не найден"; \
	fi
	@# Проверка GraphBLAS
	@if [ -f /usr/local/include/GraphBLAS.h ]; then \
		echo "✅ GraphBLAS: заголовочные файлы найдены"; \
	else \
		echo "⚠️  GraphBLAS: заголовочные файлы не найдены в /usr/local/include"; \
	fi
	@# Проверка LAGraph
	@if [ -f /usr/local/include/LAGraph.h ]; then \
		echo "✅ LAGraph: заголовочные файлы найдены"; \
	else \
		echo "⚠️  LAGraph: заголовочные файлы не найдены в /usr/local/include"; \
	fi
	@# Проверка библиотек
	@if ldconfig -p | grep -q libgraphblas; then \
		echo "✅ libgraphblas: библиотека найдена"; \
	else \
		echo "⚠️  libgraphblas: библиотека не найдена"; \
	fi
	@if ldconfig -p | grep -q liblagraph; then \
		echo "✅ liblagraph: библиотека найдена"; \
	else \
		echo "⚠️  liblagraph: библиотека не найдена"; \
	fi
	@echo ""

# ==============================================================================
# Помощь
# ==============================================================================

.PHONY: help
help:
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║              SSSP GraphBLAS Benchmark - Makefile               ║"
	@echo "╠════════════════════════════════════════════════════════════════╣"
	@echo "║  Цели:                                                         ║"
	@echo "║    make          - Сборка проекта                              ║"
	@echo "║    make test     - Запуск юнит-тестов                          ║"
	@echo "║    make run      - Запуск бенчмарка на test.mtx                ║"
	@echo "║    make run-graph GRAPH=путь - Запуск на указанном графе       ║"
	@echo "║    make debug    - Отладочная сборка                           ║"
	@echo "║    make profile  - Сборка для профилирования                   ║"
	@echo "║    make clean    - Очистка скомпилированных файлов             ║"
	@echo "║    make distclean - Полная очистка (включая результаты)        ║"
	@echo "║    make install  - Установка в систему                         ║"
	@echo "║    make check-deps - Проверка зависимостей                     ║"
	@echo "║    make help     - Показать эту справку                        ║"
	@echo "╠════════════════════════════════════════════════════════════════╣"
	@echo "║  Переменные:                                                   ║"
	@echo "║    CC=$(CC)                                      ║"
	@echo "║    CFLAGS=$(CFLAGS)                     ║"
	@echo "║    LDFLAGS=$(LDFLAGS)                         ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"

# ==============================================================================
# Конец Makefile
# ==============================================================================