#include <common.h>

node_t intersect(node_t *a, node_t a_size, node_t *b, node_t b_size, node_t *c) {
    node_t i = 0, j = 0, k = 0;
    while (i < a_size && j < b_size) {
        if (a[i] == b[j]) {
            c[k++] = a[i];
            i++;
            j++;
        }
        else if (a[i] < b[j]) {
            i++;
        }
        else {
            j++;
        }
    }
    return k;
}

node_t difference(node_t *a, node_t a_size, node_t *b, node_t b_size, node_t *c) {
    node_t i = 0, j = 0, k = 0;
    while (i < a_size && j < b_size) {
        if (a[i] == b[j]) {
            i++;
            j++;
        }
        else if (a[i] < b[j]) {
            c[k++] = a[i];
            i++;
        }
        else {
            j++;
        }
    }
    while (i < a_size) {
        c[k++] = a[i];
        i++;
    }
    return k;
}