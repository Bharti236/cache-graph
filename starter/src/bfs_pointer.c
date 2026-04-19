
#include <stdlib.h>
#include "graph.h"

int bfs_pointer(Graph* g, int source, int* dist) {
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

        /* Follow each linked-list edge and discover neighbors level by level. */
        for (Edge* edge = g->vertices[v].head; edge; edge = edge->next) {
            int u = edge->dst;
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
