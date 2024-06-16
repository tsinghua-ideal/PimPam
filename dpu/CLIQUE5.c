#include <dpu_mine.h>

#ifdef BITMAP
static ans_t __imp_clique5_bitmap(sysname_t tasklet_id, node_t second_index) {
    ans_t ans = 0;
    mram_read(mram_bitmap[second_index], bitmap[tasklet_id], sizeof(bitmap[tasklet_id]));
    for (node_t i = 0; i < bitmap_size; i++) {
        uint32_t tmp = bitmap[tasklet_id][i];
        if (tmp) for (node_t j = 0; j < 32; j++) {
            if (tmp & (1 << j)) {
                node_t third_index = (i << 5) + j;
                mram_read(mram_bitmap[third_index], bitmap[tasklet_id + NR_TASKLETS], sizeof(bitmap[tasklet_id]));
                intersect_bitmap(bitmap[tasklet_id], bitmap[tasklet_id + NR_TASKLETS], bitmap[tasklet_id + (NR_TASKLETS << 1)], bitmap_size);
                for (node_t k = 0; k < bitmap_size; k++) {
                    uint32_t tmp2 = bitmap[tasklet_id + (NR_TASKLETS << 1)][k];
                    if (tmp2) for (node_t l = 0; l < 32; l++) {
                        if (tmp2 & (1 << l)) {
                            node_t fourth_index = (k << 5) + l;
                            mram_read(mram_bitmap[fourth_index], bitmap[tasklet_id + NR_TASKLETS], sizeof(bitmap[tasklet_id]));
                            intersect_bitmap(bitmap[tasklet_id + (NR_TASKLETS << 1)], bitmap[tasklet_id + NR_TASKLETS], bitmap[tasklet_id + NR_TASKLETS], bitmap_size);
                            for (node_t m = 0; m < bitmap_size; m++) {
                                uint32_t tmp3 = bitmap[tasklet_id + NR_TASKLETS][m];
                                if (tmp3) for (node_t n = 0; n < 32; n++) {
                                    if (tmp3 & (1 << n)) ans++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return ans;
}
#endif

static ans_t __imp_clique5_2(sysname_t tasklet_id, node_t root, node_t second_root) {
    node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];

    edge_ptr root_begin = row_ptr[root];  // intended DMA
    edge_ptr root_end = row_ptr[root + 1];  // intended DMA  TODO: opt
    edge_ptr second_root_begin = row_ptr[second_root];  // intended DMA
    edge_ptr second_root_end = row_ptr[second_root + 1];  // intended DMA
    ans_t ans = 0;
    node_t common_size = intersect_seq_buf_thresh(tasklet_buf, &col_idx[root_begin], root_end - root_begin, &col_idx[second_root_begin], second_root_end - second_root_begin, mram_buf[tasklet_id], second_root);
    for (node_t j = 0; j < common_size; j++) {
        node_t third_root = mram_buf[tasklet_id][j];  // intended DMA
        edge_ptr third_root_begin = row_ptr[third_root];  // intended DMA
        edge_ptr third_root_end = row_ptr[third_root + 1];  // intended DMA
        node_t common_size2 = intersect_seq_buf_thresh(tasklet_buf, mram_buf[tasklet_id], common_size, &col_idx[third_root_begin], third_root_end - third_root_begin, mram_buf[tasklet_id + NR_TASKLETS], third_root);
        for (node_t k = 0; k < common_size2; k++) {
            node_t fourth_root = mram_buf[tasklet_id + NR_TASKLETS][k];  // intended DMA
            edge_ptr fourth_root_begin = row_ptr[fourth_root];  // intended DMA
            edge_ptr fourth_root_end = row_ptr[fourth_root + 1];  // intended DMA
            node_t common_size3 = intersect_seq_buf_thresh(tasklet_buf, mram_buf[tasklet_id + NR_TASKLETS], common_size2, &col_idx[fourth_root_begin], fourth_root_end - fourth_root_begin, mram_buf[tasklet_id + (NR_TASKLETS << 1)], fourth_root);
            ans += common_size3;
        }
    }
    return ans;
}

static ans_t __imp_clique5(sysname_t tasklet_id, node_t root) {
    edge_ptr root_begin = row_ptr[root];  // intended DMA
    edge_ptr root_end = row_ptr[root + 1];  // intended DMA
    ans_t ans = 0;
    for (edge_ptr i = root_begin; i < root_end; i++) {
        node_t second_root = col_idx[i];  // intended DMA
        if (second_root >= root) break;
        ans += __imp_clique5_2(tasklet_id, root, second_root);
    }
    return ans;
}

extern void clique5(sysname_t tasklet_id) {
    static ans_t partial_ans[NR_TASKLETS];
    static uint64_t partial_cycle[NR_TASKLETS];
    static perfcounter_cycles cycles[NR_TASKLETS];
    
    node_t i = 0;
    while (i < root_num) {
        node_t root = roots[i];  // intended DMA
        node_t root_begin = row_ptr[root];  // intended DMA
        node_t root_end = row_ptr[root + 1];  // intended DMA
        if (root_end - root_begin < BRANCH_LEVEL_THRESHOLD) {
            break;
        }
#ifdef PERF
        timer_start(&cycles[tasklet_id]);
#endif
#ifdef BITMAP
        build_bitmap(root, root_begin, root_end, tasklet_id);
#endif

        barrier_wait(&co_barrier);
        partial_ans[tasklet_id] = 0;
        for (edge_ptr j = root_begin + tasklet_id; j < root_end; j += NR_TASKLETS) {
            node_t second_root = col_idx[j];  // intended DMA
            if (second_root >= root) break;
#ifdef BITMAP
            partial_ans[tasklet_id] += __imp_clique5_bitmap(tasklet_id, j - root_begin);
#else
            partial_ans[tasklet_id] += __imp_clique5_2(tasklet_id, root, second_root);
#endif
        }
#ifdef PERF
        partial_cycle[tasklet_id] = timer_stop(&cycles[tasklet_id]);
#endif
        barrier_wait(&co_barrier);
        if (tasklet_id == 0) {
            ans_t total_ans = 0;
#ifdef PERF
            uint64_t total_cycle = 0;
#endif
            for (uint32_t j = 0; j < NR_TASKLETS; j++) {
                total_ans += partial_ans[j];
#ifdef PERF
                total_cycle += partial_cycle[j];
#endif
            }
            ans[i] = total_ans;  // intended DMA
#ifdef PERF
            cycle_ct[i] = total_cycle;  // intended DMA
#endif
        }
        i++;
    }

    for (i += tasklet_id; i < root_num; i += NR_TASKLETS) {
        node_t root = roots[i];  // intended DMA
#ifdef PERF
        timer_start(&cycles[tasklet_id]);
#endif
        ans[i] = __imp_clique5(tasklet_id, root);  // intended DMA
#ifdef PERF
        cycle_ct[i] = timer_stop(&cycles[tasklet_id]);  // intended DMA
#endif
    }
}