# SSSP GraphBLAS

## О проекте

Реализация и сравнительный анализ различных алгоритмов **Single-Source Shortest Path (SSSP)** с использованием:

- **SuiteSparse GraphBLAS** — низкоуровневая линейная алгебра для графов
- **LAGraph** — высокоуровневая библиотека готовых графовых алгоритмов

### Цели проекта

1. Реализовать 3 алгоритма SSSP на чистом GraphBLAS
2. Сравнить производительность с готовыми решениями LAGraph
3. Протестировать на реальных графах из SuiteSparse Matrix Market Collection
4. Предоставить воспроизводимые бенчмарки для научного использования

---

## Алгоритмы

### 1. Bellman-Ford (LAGraph)
Готовая реализация из LAGraph. Поддерживает отрицательные веса рёбер.
```c
bellman_ford_lagraph(BF_Result *result, LAGraph_Graph graph, GrB_Index source)
```

### 2. Dijkstra (LAGraph)
Классический алгоритм Дейкстры через приоритетную очередь. Только неотрицательные веса.
```c
dijkstra_lagraph(Dijkstra_Result *result, LAGraph_Graph graph, GrB_Index source)
```

### 3. Delta-Stepping (LAGraph)
Параллельный алгоритм, оптимальный для разреженных графов. Требует параметр Δ.
```c
delta_stepping_lagraph(Delta_Result *result, LAGraph_Graph graph, GrB_Index source, double delta)
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
