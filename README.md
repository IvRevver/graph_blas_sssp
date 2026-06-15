# SSSP GraphBLAS
[![Build & Test](https://github.com/ivrevver/sssp-graphblas-benchmark/actions/workflows/build.yml/badge.svg)](https://github.com/yourusername/sssp-graphblas-benchmark/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

## О проекте

Реализация и сравнительный анализ различных алгоритмов **Single-Source Shortest Path (SSSP)** с использованием:

- **SuiteSparse GraphBLAS** — низкоуровневая линейная алгебра для графов
- **LAGraph** — высокоуровневая библиотека готовых графовых алгоритмов

### Цели проекта

1. Реализовать 2 алгоритма SSSP на чистом GraphBLAS
2. Сравнить производительность с готовыми решениями (LAGraph)
3. Протестировать на реальных графах из SuiteSparse Matrix Market Collection
4. Предоставить воспроизводимые бенчмарки для научного использования

---

## Описание проекта

Реализованы **3 алгоритма SSSP**:

| # | Алгоритм | Реализация | Файл | Отрицательные рёбра |
|---|----------|------------|------|---------------------|
| 1 | Delta-Stepping | LAGraph (`LAGr_SingleSourceShortestPath`) | `src/algorithms/lagraph_sssp.c` | ❌ |
| 2 | Algebraic Bellman-Ford | Raw GraphBLAS (min-plus) | `src/algorithms/algebraic_bf_graphblas.c` | ✅ |
| 3 | Dijkstra | Raw GraphBLAS + priority queue | `src/algorithms/dijkstra_graphblas.c` | ❌ |

---

## Требования

| Компонент | Минимальная версия |
|-----------|-------------------|
| GCC (C11) | 7+ |
| SuiteSparse GraphBLAS | 7.0+ |
| LAGraph | 1.0+ |
| OpenMP | 4.0+ |
| GNU Make | 4.0+ |

## Установка зависимостей (Ubuntu/Debian)

```bash
bash scripts/install_deps.sh
```

**Или вручную:**

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git libopenmp-dev libsuitesparse-dev

# GraphBLAS
git clone --depth 1 https://github.com/DrTimothyAldenDavis/GraphBLAS.git /tmp/GraphBLAS
cd /tmp/GraphBLAS
make -j$(nproc)
sudo make install
sudo ldconfig

# LAGraph
git clone --depth 1 https://github.com/GraphBLAS/LAGraph.git /tmp/LAGraph
cd /tmp/LAGraph
make -j$(nproc)
sudo make install
sudo ldconfig
```

## Сборка

```bash
make
```

Исполняемый файл: `bin/sssp_benchmark`

## Запуск

```bash
# На тестовом графе
make run

# На произвольном графе
make run-graph GRAPH=graphs/lesmis.mtx

# С параметрами
make run-graph GRAPH=graphs/lesmis.mtx SOURCE=5 DELTA=4.0

# Напрямую
./bin/sssp_benchmark graphs/test.mtx 0 3.0
```

**Параметры командной строки:**

| Параметр | Описание | По умолчанию |
|----------|----------|-------------|
| `<файл.mtx>` | Путь к графу (Matrix Market) | Обязательно |
| `source` | Исходная вершина | 0 |
| `delta` | Параметр Delta-Stepping | 3.0 |

**Переменные Makefile:**

| Переменная | По умолчанию | Описание |
|-----------|-------------|----------|
| `GRAPH` | — | Путь к файлу `.mtx` |
| `SOURCE` | `0` | Исходная вершина |
| `DELTA` | `3.0` | Параметр Delta-Stepping |

## Тесты

```bash
make test
```

Исполняемый файл: `bin/test_sssp`

## Цели Makefile

| Цель | Описание |
|------|----------|
| `make` | Сборка проекта |
| `make test` | Запуск юнит-тестов |
| `make run` | Бенчмарк на `graphs/test.mtx` |
| `make run-graph GRAPH=f.mtx` | Бенчмарк на указанном графе |
| `make debug` | Отладочная сборка (`-g -DDEBUG`) |
| `make profile` | Сборка с профилированием (`-pg`) |
| `make clean` | Удаление `obj/`, `bin/` |
| `make distclean` | Полная очистка (включая `results/`) |
| `make install` | Установка в `/usr/local/bin` |
| `make check-deps` | Проверка зависимостей |
| `make help` | Справка |

## Формат входных данных

Формат **Matrix Market coordinate real general**.

```
%%MatrixMarket matrix coordinate real general
6 6 10
1 2 2
1 3 4
...
```

- Строка 1: заголовок
- Строка 2: `nrows ncols nentries`
- Строки 3+: `row col value` (1-индексированные, конвертируются в 0-индексацию в `graph_loader.c`)

## Валидация

Модуль `src/utils/validator.c` (`VALIDATOR_EPSILON = 1e-6`):

| Функция | Проверка |
|---------|----------|
| `sssp_validate_source_distance` | `dist[source] == 0` |
| `sssp_validate_non_negative` | Все расстояния ≥ 0 или ∞ |
| `sssp_validate_distances` | Согласованность двух алгоритмов |
| `sssp_validate_result` | Все проверки + `success == true` |

## Структура проекта

```
├── src/
│   ├── main.c
│   ├── algorithms/
│   │   ├── ssp_common.{h,c}
│   │   ├── lagraph_sssp.{h,c}
│   │   ├── algebraic_bf_graphblas.{h,c}
│   │   └── dijkstra_graphblas.{h,c}
│   ├── graph/
│   │   └── graph_loader.{h,c}
│   └── utils/
│       ├── timer.{h,c}
│       └── validator.{h,c}
├── tests/
│   ├── test_all.c
│   └── test_graph.mtx
├── graphs/
│   └── test.mtx
├── scripts/
│   └── install_deps.sh
├── Makefile
└── .gitignore
```

---

## Ссылки и ресурсы

### Документация

| Ресурс | Ссылка |
|--------|--------|
| GraphBLAS Specification | [graphblas.org](https://graphblas.org/docs/) |
| LAGraph Documentation | [GitHub LAGraph](https://github.com/GraphBLAS/LAGraph) |
| SuiteSparse Collection | [suitesparse-collection-website](https://suitesparse-collection-website.herokuapp.com/) |

### Научные статьи

1. **Davis, T. A.** (2019). *GraphBLAS: A Graph Algebra in Linear Algebra*. SIAM News.
2. **Kepner, J. et al.** (2018). *Mathematical Foundations of the GraphBLAS*. IEEE HPEC.
3. **Meyer, U. & Sanders, P.** (2003). *Δ-stepping: A Parallelizable Shortest Path Algorithm*. J. Algorithms.

## Лицензия

MIT. См. файл `LICENSE`.
