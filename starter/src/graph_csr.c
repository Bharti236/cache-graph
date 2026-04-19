
#include <stdlib.h>
#include "graph.h"

CSRGraph* convert_to_csr(Graph* g) {
    if (!g) {
        return NULL;
    }

    int n = g->num_vertices;
    int edge_count = 0;

    for (int v = 0; v < n; v++) {
        for (Edge* edge = g->vertices[v].head; edge; edge = edge->next) {
            edge_count++;
        }
    }

    CSRGraph* csr = (CSRGraph*)malloc(sizeof(CSRGraph));
    if (!csr) {
        return NULL;
    }

    csr->num_vertices = n;
    csr->num_edges = edge_count;
    csr->row_ptr = (int*)malloc((size_t)(n + 1) * sizeof(int));
    csr->col_idx = edge_count > 0 ? (int*)malloc((size_t)edge_count * sizeof(int)) : NULL;

    if (!csr->row_ptr || (edge_count > 0 && !csr->col_idx)) {
        free_csr(csr);
        return NULL;
    }

    int next_edge = 0;
    for (int v = 0; v < n; v++) {
        csr->row_ptr[v] = next_edge;

        /* Copy each adjacency list into CSR's compact contiguous edge array. */
        for (Edge* edge = g->vertices[v].head; edge; edge = edge->next) {
            csr->col_idx[next_edge++] = edge->dst;
        }
    }
    csr->row_ptr[n] = next_edge;

    return csr;
}

void free_csr(CSRGraph* g) {
    if (!g) return;
    free(g->row_ptr);
    free(g->col_idx);
    free(g);
}
