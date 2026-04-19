# Cache-Friendly Graph Processing Report

## Summary

This assignment is now implemented in the required starter code. I completed the three missing functions:

- `src/bfs_pointer.c`
- `src/graph_csr.c`
- `src/bfs_csr.c`

I also added short code comments in the key traversal and conversion loops so the logic is easier to follow.

The finished code:

- builds successfully with `make`
- passes all provided visible BFS correctness tests
- shows a clear runtime advantage for the CSR representation on a larger generated graph

## What I Implemented

### 1. BFS on the pointer-based graph

File: `src/bfs_pointer.c`

Implemented a standard breadth-first search over the linked-list adjacency representation.

Behavior:

- validates inputs
- initializes every entry in `dist` to `-1`
- uses a queue allocated to hold up to `n` vertices
- sets `dist[source] = 0`
- visits each reachable vertex once
- returns the number of visited vertices

This version follows each neighbor by pointer chasing through `Edge* next`, which is correct but less cache-friendly.

### 2. Conversion from adjacency lists to CSR

File: `src/graph_csr.c`

Implemented `convert_to_csr(Graph* g)` in two passes:

1. Count all edges in the pointer-based graph.
2. Allocate and fill:
   - `row_ptr` of size `n + 1`
   - `col_idx` of size `m`

The resulting CSR representation stores each vertex's neighbors in one contiguous slice:

```text
neighbors of v are in col_idx[row_ptr[v] ... row_ptr[v+1)-1]
```

This gives much better spatial locality than the linked-list representation.

### 3. BFS on the CSR graph

File: `src/bfs_csr.c`

Implemented the same BFS algorithm over the CSR layout.

Behavior:

- validates inputs
- initializes `dist` to `-1`
- uses a queue for level-order traversal
- iterates neighbors through contiguous array indices:

```text
for (i = row_ptr[v]; i < row_ptr[v + 1]; i++)
```

Because the adjacency data is contiguous, this traversal is typically much friendlier to the CPU cache.

## Correctness Validation

I built the project and ran the provided checker:

```bash
make
python3 visible_checker.py /home/bharti/cache_graph/starter
```

Result: all visible tests passed for both implementations.

Passed cases:

- `test_small.txt`
- `test_chain.txt`
- `test_star.txt`
- `test_disconnected.txt`
- `test_binary_tree.txt`
- `test_duplicate_edges.txt`
- `test_cycle.txt`
- `test_two_components.txt`

That confirms:

- shortest-path distances are computed correctly
- unreachable nodes remain `-1`
- duplicate edges do not break BFS
- cycles and disconnected components are handled correctly

## Runtime Comparison

### Benchmark setup

I generated a larger Erdos-Renyi style graph with:

```bash
python3 scripts/gen_graph.py --kind er --n 50000 --deg 8 --seed 1 --out /tmp/cache_graph_er_50000_d8.txt
```

Then I benchmarked both implementations with the provided program:

```bash
./graph_bench --impl=pointer --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=500
./graph_bench --impl=csr --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=500
```

Observed output:

| Implementation | Visited | Total time for 500 runs | Approx. time per BFS |
| --- | ---: | ---: | ---: |
| Pointer graph | 49983 | 7675.59 ms | 15.351 ms |
| CSR graph | 49983 | 1527.76 ms | 3.056 ms |

### Answer to Question 1

CSR was faster.

- Speedup: about `5.02x`
- Time reduction: about `80.1%`

This is especially notable because the provided benchmark includes the one-time `convert_to_csr(...)` cost in the CSR timing path. That means the BFS traversal itself is likely even more favorable to CSR when the same converted graph is reused for many searches.

## Cache Behavior Analysis

### Environment limitation

The assignment asks for Cachegrind analysis, but this environment does not have `valgrind` installed:

```bash
valgrind --version
```

Result:

```text
/bin/bash: line 1: valgrind: command not found
```

Because of that, I could not produce measured `D1 misses` and `LLd misses` in this workspace.

### Answer to Question 2

Even without Cachegrind numbers, the implementation strongly suggests that CSR should generate fewer cache misses than the pointer-based graph.

Reason:

- The pointer graph stores each edge in a separately allocated linked-list node.
- BFS on that layout repeatedly follows `next` pointers, which can jump to unrelated memory locations.
- CSR stores all edges in one compact integer array.
- When BFS scans neighbors in CSR, it reads memory sequentially, which fits how hardware caches and prefetchers work.

Expected outcome:

- CSR should have fewer `D1` misses than the pointer graph.
- CSR should also usually have fewer `LLd` misses, especially on larger graphs where adjacency data no longer fits in the smallest caches.

## Memory Layout Explanation

### Answer to Question 3

CSR tends to perform better because it improves spatial locality.

In the pointer graph:

- each edge is a heap-allocated node
- adjacency traversal requires pointer chasing
- consecutive neighbor visits are not guaranteed to be near each other in memory

In CSR:

- `row_ptr` and `col_idx` are contiguous arrays
- scanning a vertex's neighbors becomes a simple sequential walk over integers
- contiguous accesses are easier for cache lines to capture efficiently
- the CPU can often prefetch upcoming data

So even though both BFS implementations are still `O(V + E)`, CSR usually runs faster in practice because it makes memory access more predictable and cache-friendly.

## Varying Cache Parameters

### Answer to Question 4

I could not run the required cache-configuration experiments in this environment because Cachegrind is unavailable. However, the expected trends are:

1. Increasing cache size should generally help both implementations, but especially the pointer graph, because more scattered edge nodes can remain cached.
2. Increasing associativity should reduce conflict misses, which can help both versions, though the effect is often smaller than changing layout from pointer lists to CSR.
3. Increasing cache line size should benefit CSR more, because adjacent neighbors are stored contiguously and a single cache line fetch brings in several useful entries.
4. CSR should benefit more from larger cache lines than pointer graphs, because the pointer graph's next useful edge is often not adjacent in memory.

## Final Notes

- I did not modify the files that the README marked as off-limits (`graph_loader.c`, `benchmark.c`, `main.c`).
- The required source files are complete and compile cleanly with the provided Makefile.
- The code now contains short comments in the important logic paths to improve readability.

## Reproducibility Commands

```bash
cd /home/bharti/cache_graph/starter
make clean && make
python3 /home/bharti/cache_graph/visible_checker.py /home/bharti/cache_graph/starter
python3 scripts/gen_graph.py --kind er --n 50000 --deg 8 --seed 1 --out /tmp/cache_graph_er_50000_d8.txt
./graph_bench --impl=pointer --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=500
./graph_bench --impl=csr --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=500
```

If `valgrind` is available on another machine, these are the commands to complete the cache-study section:

```bash
valgrind --tool=cachegrind ./graph_bench --impl=pointer --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=50
valgrind --tool=cachegrind ./graph_bench --impl=csr --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=50
valgrind --tool=cachegrind --D1=16384,4,64 ./graph_bench --impl=pointer --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=50
valgrind --tool=cachegrind --D1=16384,4,64 ./graph_bench --impl=csr --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=50
valgrind --tool=cachegrind --D1=32768,8,64 ./graph_bench --impl=pointer --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=50
valgrind --tool=cachegrind --D1=32768,8,64 ./graph_bench --impl=csr --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=50
```
