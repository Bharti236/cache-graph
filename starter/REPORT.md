# Cache-Friendly Graph Processing Report

## Summary

This assignment is fully implemented in the required starter code. I completed the three missing functions:

- `src/bfs_pointer.c`
- `src/graph_csr.c`
- `src/bfs_csr.c`

The finished code:

- builds successfully with `make`
- passes all provided visible BFS correctness tests
- passes small Valgrind Memcheck smoke tests with no leaks or memory errors
- shows a clear runtime and cache-miss advantage for the CSR representation

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

I also ran small Valgrind Memcheck smoke tests for both implementations:

```bash
valgrind --tool=memcheck --leak-check=full --errors-for-leak-kinds=all ./graph_bench --impl=pointer --graph=tests/test_small.txt --source=0
valgrind --tool=memcheck --leak-check=full --errors-for-leak-kinds=all ./graph_bench --impl=csr --graph=tests/test_small.txt --source=0
```

Both runs reported:

- `ERROR SUMMARY: 0 errors from 0 contexts`
- `All heap blocks were freed -- no leaks are possible`

## Runtime Comparison

### Benchmark setup

I generated a larger Erdos-Renyi style graph with:

```bash
python3 scripts/gen_graph.py --kind er --n 50000 --deg 8 --seed 1 --out /tmp/cache_graph_er_50000_d8.txt
```

Then I benchmarked both implementations three times:

```bash
./graph_bench --impl=pointer --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=500
./graph_bench --impl=csr --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=500
```

Observed results:

| Implementation | Run 1 total | Run 2 total | Run 3 total | Average total | Approx. time per BFS |
| --- | ---: | ---: | ---: | ---: | ---: |
| Pointer graph | 2905.49 ms | 2614.34 ms | 2609.53 ms | 2709.79 ms | 5.420 ms |
| CSR graph | 813.24 ms | 850.13 ms | 793.18 ms | 818.85 ms | 1.638 ms |

### Answer to Question 1

CSR was faster.

- Average speedup: about `3.31x`
- Average time reduction: about `69.8%`

This is still notable because the provided benchmark includes the one-time `convert_to_csr(...)` cost in the CSR timing path. If the same converted graph were reused for many BFS traversals, CSR would likely look even better.

## Cache Behavior Analysis

### Benchmark setup

To make the Cachegrind runs reproducible, I used an explicit L1 data-cache configuration instead of relying on host defaults:

```bash
valgrind --tool=cachegrind --cache-sim=yes --D1=32768,8,64 ./graph_bench --impl=pointer --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=10
valgrind --tool=cachegrind --cache-sim=yes --D1=32768,8,64 ./graph_bench --impl=csr --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=10
```

Miss totals under that configuration:

| Implementation | D1 misses | LLd misses |
| --- | ---: | ---: |
| Pointer graph | 7,195,870 | 1,802,208 |
| CSR graph | 5,889,600 | 769,233 |

### Answer to Question 2

CSR generated fewer data-cache misses at both levels.

- D1 misses dropped by about `18.2%`
- LLd misses dropped by about `57.3%`

The reason is that CSR stores adjacency data in contiguous arrays, so BFS can scan neighbors sequentially. The pointer representation must chase linked-list nodes that were allocated separately on the heap, which leads to less predictable memory access and more cache misses.

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

Cachegrind is a simulator, so for this section I treat lower miss counts as the main performance signal rather than the `time_ms` printed during a Valgrind run.

### Measured configurations

| D1 configuration | Pointer D1 misses | CSR D1 misses | Pointer LLd misses | CSR LLd misses |
| --- | ---: | ---: | ---: | ---: |
| `16KB, 8-way, 64B` | 7,432,577 | 6,184,946 | 1,802,259 | 769,234 |
| `32KB, 2-way, 64B` | 8,627,849 | 7,326,886 | 1,802,276 | 769,404 |
| `32KB, 4-way, 64B` | 7,202,990 | 5,896,332 | 1,802,260 | 769,268 |
| `32KB, 8-way, 64B` | 7,195,870 | 5,889,600 | 1,802,208 | 769,233 |
| `32KB, 8-way, 32B` | 9,368,196 | 7,144,825 | 1,802,219 | 769,250 |
| `32KB, 8-way, 128B` | 5,936,446 | 5,266,515 | 817,227 | 400,535 |

### Answer to Question 4

1. Increasing cache size from `16KB` to `32KB` reduced D1 misses for both implementations, but only modestly.
   - Pointer: `7,432,577` -> `7,195,870` (`-3.2%`)
   - CSR: `6,184,946` -> `5,889,600` (`-4.8%`)

   LLd misses were almost unchanged, so the larger L1 helped mainly by absorbing some extra first-level misses rather than changing last-level behavior.

2. Increasing associativity helped by reducing conflict misses.
   - Pointer D1 misses at `32KB, 64B`: `8,627,849` (2-way) -> `7,202,990` (4-way) -> `7,195,870` (8-way)
   - CSR D1 misses at `32KB, 64B`: `7,326,886` (2-way) -> `5,896,332` (4-way) -> `5,889,600` (8-way)

   Most of the benefit came from moving off 2-way associativity. The step from 4-way to 8-way helped only a little more.

3. Increasing cache line size helped both implementations.
   - Pointer D1 misses at `32KB, 8-way`: `9,368,196` (32B) -> `7,195,870` (64B) -> `5,936,446` (128B)
   - CSR D1 misses at `32KB, 8-way`: `7,144,825` (32B) -> `5,889,600` (64B) -> `5,266,515` (128B)

   Larger cache lines also cut LLd misses substantially at `128B`, because each miss brings in more neighboring data. This workload has enough spatial locality for larger lines to pay off.

4. In these measured whole-program runs, CSR did **not** benefit more than the pointer graph in percentage terms from larger cache lines. The pointer version improved more because it started from a much worse miss profile, so it had more room to improve. However, CSR still had fewer misses than the pointer graph at every tested line size, so it remained the more cache-friendly layout overall.

## Final Notes

- I did not modify the files that the README marked as off-limits (`graph_loader.c`, `benchmark.c`, `main.c`).
- The required source files are complete and compile cleanly with the provided Makefile.
- The code contains short comments in the important logic paths to improve readability.

## Reproducibility Commands

```bash
cd /home/bharti/cache_graph/starter
make clean && make
python3 /home/bharti/cache_graph/visible_checker.py /home/bharti/cache_graph/starter
python3 scripts/gen_graph.py --kind er --n 50000 --deg 8 --seed 1 --out /tmp/cache_graph_er_50000_d8.txt

./graph_bench --impl=pointer --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=500
./graph_bench --impl=csr --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=500

valgrind --tool=memcheck --leak-check=full --errors-for-leak-kinds=all ./graph_bench --impl=pointer --graph=tests/test_small.txt --source=0
valgrind --tool=memcheck --leak-check=full --errors-for-leak-kinds=all ./graph_bench --impl=csr --graph=tests/test_small.txt --source=0

valgrind --tool=cachegrind --cache-sim=yes --D1=32768,8,64 ./graph_bench --impl=pointer --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=10
valgrind --tool=cachegrind --cache-sim=yes --D1=32768,8,64 ./graph_bench --impl=csr --graph=/tmp/cache_graph_er_50000_d8.txt --source=0 --repeat=10
```
