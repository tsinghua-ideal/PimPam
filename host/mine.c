#include <common.h>

node_t buf[32][MRAM_BUF_SIZE];

extern node_t intersect(node_t *a, node_t a_size, node_t *b, node_t b_size, node_t *c);
extern node_t difference(node_t *a, node_t a_size, node_t *b, node_t b_size, node_t *c);

ans_t clique2(Graph *g, node_t root) {
    edge_ptr root_begin = g->row_ptr[root];
    edge_ptr root_end = g->row_ptr[root + 1];
    ans_t ans = 0;
    for (edge_ptr i = root_begin; i < root_end; i++) {
        if (g->col_idx[i] >= root) break;
        ans++;
    }
    return ans;
}

ans_t clique3(Graph *g, node_t root) {
    edge_ptr root_begin = g->row_ptr[root];
    edge_ptr root_end = g->row_ptr[root + 1];
    ans_t ans = 0;
    for (edge_ptr i = root_begin; i < root_end; i++) {
        node_t second_root = g->col_idx[i];
        if (second_root >= root) break;
        edge_ptr second_root_begin = g->row_ptr[second_root];
        edge_ptr second_root_end = g->row_ptr[second_root + 1];
        node_t common_size = intersect(&g->col_idx[root_begin], root_end - root_begin, &g->col_idx[second_root_begin], second_root_end - second_root_begin, buf[0]);
        for (node_t j = 0; j < common_size; j++) {
            if (buf[0][j] >= second_root) break;
            ans++;
        }
    }
    return ans;
}

ans_t clique4(Graph *g, node_t root) {
    edge_ptr root_begin = g->row_ptr[root];
    edge_ptr root_end = g->row_ptr[root + 1];
    ans_t ans = 0;
    for (edge_ptr i = root_begin; i < root_end; i++) {
        node_t second_root = g->col_idx[i];
        if (second_root >= root) break;
        edge_ptr second_root_begin = g->row_ptr[second_root];
        edge_ptr second_root_end = g->row_ptr[second_root + 1];
        node_t common_size = intersect(&g->col_idx[root_begin], root_end - root_begin, &g->col_idx[second_root_begin], second_root_end - second_root_begin, buf[0]);
        for (node_t j = 0; j < common_size; j++) {
            node_t third_root = buf[0][j];
            if (third_root >= second_root) break;
            edge_ptr third_root_begin = g->row_ptr[third_root];
            edge_ptr third_root_end = g->row_ptr[third_root + 1];
            node_t common_size2 = intersect(buf[0], common_size, &g->col_idx[third_root_begin], third_root_end - third_root_begin, buf[1]);
            for (node_t k = 0; k < common_size2; k++) {
                if (buf[1][k] >= third_root) break;
                ans++;
            }
        }
    }
    return ans;
}

ans_t cycle4(Graph *g, node_t root) {
    edge_ptr root_begin = g->row_ptr[root];
    edge_ptr root_end = g->row_ptr[root + 1];
    ans_t ans = 0;
    for (edge_ptr i = root_begin; i < root_end; i++) {
        node_t second_root = g->col_idx[i];
        if (second_root >= root) break;
        edge_ptr second_root_begin = g->row_ptr[second_root];
        edge_ptr second_root_end = g->row_ptr[second_root + 1];
        for (edge_ptr j = root_begin; j < root_end; j++) {
            node_t third_root = g->col_idx[j];
            if (third_root >= second_root) break;
            edge_ptr third_root_begin = g->row_ptr[third_root];
            edge_ptr third_root_end = g->row_ptr[third_root + 1];
            node_t common_size = intersect(&g->col_idx[second_root_begin], second_root_end - second_root_begin, &g->col_idx[third_root_begin], third_root_end - third_root_begin, buf[1]);
            for (node_t k = 0; k < common_size; k++) {
                if (buf[1][k] >= root) break;
                ans++;
            }
        }
    }
    return ans;
}

ans_t house5(Graph *g, node_t root) {
    edge_ptr root_begin = g->row_ptr[root];
    edge_ptr root_end = g->row_ptr[root + 1];
    ans_t ans = 0;
    for (edge_ptr i = root_begin; i < root_end; i++) {
        node_t second_root = g->col_idx[i];
        if (second_root >= root) break;
        edge_ptr second_root_begin = g->row_ptr[second_root];
        edge_ptr second_root_end = g->row_ptr[second_root + 1];
        node_t fifth_root_size = intersect(&g->col_idx[root_begin], root_end - root_begin, &g->col_idx[second_root_begin], second_root_end - second_root_begin, buf[0]);
        for (edge_ptr j = root_begin; j < root_end; j++) {
            node_t third_root = g->col_idx[j];
            if (third_root == second_root) continue;
            edge_ptr third_root_begin = g->row_ptr[third_root];
            edge_ptr third_root_end = g->row_ptr[third_root + 1];
            node_t fourth_root_size = intersect(&g->col_idx[second_root_begin], second_root_end - second_root_begin, &g->col_idx[third_root_begin], third_root_end - third_root_begin, buf[1]);
            node_t cur_fifth = fifth_root_size;
            for (node_t k = 0; k < fifth_root_size; k++) {
                if (buf[0][k] == third_root) {
                    cur_fifth--;
                    break;
                }
            }
            node_t common_size = intersect(buf[0], fifth_root_size, buf[1], fourth_root_size, buf[2]);
            ans += ((ans_t)cur_fifth) * (fourth_root_size - 1) - common_size;
        }
    }
    return ans;
}

ans_t tri_tri6(Graph *g, node_t root) {
    edge_ptr root_begin = g->row_ptr[root];
    edge_ptr root_end = g->row_ptr[root + 1];
    ans_t ans = 0;
    for (edge_ptr i = root_begin; i < root_end; i++) {
        node_t second_root = g->col_idx[i];
        if (second_root >= root) break;
        edge_ptr second_root_begin = g->row_ptr[second_root];
        edge_ptr second_root_end = g->row_ptr[second_root + 1];
        node_t common_size = intersect(&g->col_idx[root_begin], root_end - root_begin, &g->col_idx[second_root_begin], second_root_end - second_root_begin, buf[0]);
        for (node_t j = 0; j < common_size; j++) {
            node_t third_root = buf[0][j];
            if (third_root >= second_root) break;
            edge_ptr third_root_begin = g->row_ptr[third_root];
            edge_ptr third_root_end = g->row_ptr[third_root + 1];
            node_t common_size2 = intersect(&g->col_idx[second_root_begin], second_root_end - second_root_begin, &g->col_idx[third_root_begin], third_root_end - third_root_begin, buf[1]);
            node_t common_size3 = intersect(&g->col_idx[root_begin], root_end - root_begin, &g->col_idx[third_root_begin], third_root_end - third_root_begin, buf[2]);
            node_t common_size123 = intersect(buf[0], common_size, buf[1], common_size2, buf[3]);
            ans += ((ans_t)common_size - 1) * (common_size2 - 1) * (common_size3 - 1) - ((ans_t)common_size123) * (((ans_t)common_size) + common_size2 + common_size3 - 5);
        }
    }
    return ans;
}