# SSSP GraphBLAS
[![CI](https://github.com/IvRevver/graph_blas_sssp/actions/workflows/ci.yml/badge.svg)](https://github.com/IvRevver/graph_blas_sssp/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

Реализация и сравнительный анализ алгоритмов **Single-Source Shortest Path (SSSP)** на базе SuiteSparse GraphBLAS и LAGraph.

| # | Алгоритм | Реализация | Файл |
|---|----------|------------|------|
| 1 | Delta-Stepping | LAGraph | `src/algorithms/lagraph_sssp.c` |
| 2 | Algebraic Bellman-Ford | Raw GraphBLAS (min-plus) | `src/algorithms/algebraic_bf_graphblas.c` |
| 3 | Dijkstra | Raw GraphBLAS + priority queue | `src/algorithms/dijkstra_graphblas.c` |

## Сборка

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Исполняемые файлы: `bin/sssp_benchmark`, `bin/test_sssp`.

## Запуск

### Режимы

| Режим | Команда | Описание |
|-------|---------|----------|
| Однократный | `sssp_benchmark <file> <source> <delta>` | Один замер, вывод результатов |
| Статистический | `sssp_benchmark <file> <source> <delta> <runs> [-w N]` | N замеров на алгоритм, вывод mean/min/max/std + CSV в `results/`. Перед замерами `-w N` прогревочных запусков (по умолч. 1). |
| Полный | `sssp_benchmark <file> -b <source> <runs> [-w N]` | Прогон для всех дельт (0.5, 1, 2, 3, 5, 10) с прогревами |

Примеры:

```bash
./bin/sssp_benchmark graphs/test.mtx 0 3.0
./bin/sssp_benchmark graphs/test.mtx 0 3.0 30 -w 3
./bin/sssp_benchmark graphs/test.mtx -b 0 30 -w 5
```

### Параметры

| Параметр | Описание | По умолчанию |
|----------|----------|-------------|
| `<файл.mtx>` | Путь к графу (Matrix Market) | Обязательно |
| `source` | Исходная вершина | 0 |
| `delta` | Параметр Delta-Stepping (не для `-b`) | 3.0 |
| `runs` | Количество замеров | 1 |
| `-w N` | Прогревочные запуски перед замерами | 1 |

Полный режим (`-b`) перебирает все дельты и записывает `results/<граф>_raw_<алгоритм>.csv` с сырыми замерами и `results/<граф>_summary.csv` со сводкой. По умолчанию перед замерами выполняется 1 прогревочный запуск (`-w 1`) — его результат не учитывается. Отключить прогревы: `-w 0`.

## Тесты

```bash
cmake --build build --target test_sssp
ctest --test-dir build
```

## Требования

| Компонент | Версия |
|-----------|--------|
| GCC (C11) | 7+ |
| SuiteSparse GraphBLAS | 7.0+ |
| LAGraph | 1.0+ |
| CMake | 3.13+ |

### Установка зависимостей

**MSYS2 (MinGW64):**

```bash
pacman -S mingw-w64-x86_64-suitesparse
```

**Ubuntu/Debian (из исходников):**

```bash
sudo apt-get install -y build-essential cmake git

git clone --depth 1 --branch v10.3.1 https://github.com/DrTimothyAldenDavis/GraphBLAS.git
make -j$(nproc) -C GraphBLAS
sudo make -C GraphBLAS install

git clone --depth 1 --branch v1.2.1 https://github.com/GraphBLAS/LAGraph.git
cmake -S LAGraph -B LAGraph/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc) -C LAGraph/build
sudo make -C LAGraph/build install
```

## Формат входных данных

Matrix Market coordinate real general (1-индексированный, конвертируется в 0 внутри `graph_loader.c`):

```
%%MatrixMarket matrix coordinate real general
6 6 10
1 2 2
1 3 4
...
```

## Структура проекта

```
├── src/algorithms/       # Библиотека алгоритмов (sssp_algorithms)
├── benchmarks/           # Загрузка графов, замеры, валидация (sssp_benchmark)
│   ├── graph/
│   └── utils/
├── tests/                # Юнит-тесты (test_sssp)
├── graphs/               # Входные графы (.mtx)
├── CMakeLists.txt
└── bin/                  # Исполняемые файлы (после сборки)
```

## Лицензия

MIT. См. файл `LICENSE`.
