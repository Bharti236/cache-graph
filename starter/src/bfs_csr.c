
#include <stdlib.h>
#include "graph.h"

int bfs_csr(CSRGraph* g, int source, int* dist) {
    if (!g || !dist || source < 0 || source >= g->num_vertices) {
        return -1;
    }

    int n = g->num_vertices;
    for (int v = 0; v < n; v++) {
        dist[v] = -1;
    }

    int* queue = (int*)malloc((size_t)n * sizeof(int));
    if (!queue) {
        return -1;
    }

    int head = 0;
    int tail = 0;
    int visited = 1;

    dist[source] = 0;
    queue[tail++] = source;

    while (head < tail) {
        int v = queue[head++];

        /* CSR stores each vertex's neighbors in one contiguous slice. */
        for (int i = g->row_ptr[v]; i < g->row_ptr[v + 1]; i++) {
            int u = g->col_idx[i];
            if (dist[u] != -1) {
                continue;
            }

            dist[u] = dist[v] + 1;
            queue[tail++] = u;
            visited++;
        }
    }

    free(queue);
    return visited;
}
