#include <dpu_mine.h>

static ans_t __imp_house5_2(sysname_t tasklet_id, node_t root, node_t second_root, node_t start, node_t step) {
    node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];

    edge_ptr root_begin = row_ptr[root];  // intended DMA
    edge_ptr root_end = row_ptr[root + 1];  // intended DMA TODO: opt
    edge_ptr second_root_begin = row_ptr[second_root];  // intended DMA
    edge_ptr second_root_end = row_ptr[second_root + 1];  // intended DMA
    ans_t ans = 0;
    node_t fifth_root_size = intersect_seq_buf_thresh(tasklet_buf, &col_idx[second_root_begin], second_root_end - second_root_begin, &col_idx[root_begin], root_end - root_begin, mram_buf[tasklet_id + NR_TASKLETS], INVALID_NODE);
    node_t cur_cmp = 0;
    if (fifth_root_size) for (edge_ptr j = root_begin + start; j < root_end; j += step) {
        node_t third_root = col_idx[j];  // intended DMA
        if (third_root == second_root) continue;
        edge_ptr third_root_begin = row_ptr[third_root];  // intended DMA
        edge_ptr third_root_end = row_ptr[third_root + 1];  // intended DMA
        node_t fourth_root_size = intersect_seq_buf_thresh(tasklet_buf, &col_idx[second_root_begin], second_root_end - second_root_begin, &col_idx[third_root_begin], third_root_end - third_root_begin, mram_buf[tasklet_id], INVALID_NODE);
        if (!fourth_root_size) continue;
        node_t common_size = intersect_seq_buf_thresh(tasklet_buf, mram_buf[tasklet_id], fourth_root_size, mram_buf[tasklet_id + NR_TASKLETS], fifth_root_size, mram_buf[tasklet_id + (NR_TASKLETS << 1)], INVALID_NODE);
        node_t cur_fifth = fifth_root_size;
        while (cur_cmp < fifth_root_size) {
            node_t fifth_root = mram_buf[tasklet_id + NR_TASKLETS][cur_cmp];  // intended DMA
            if (fifth_root > third_root) break;
            else if (fifth_root == third_root) {
                cur_fifth--;
                cur_cmp++;
                break;
            }
            cur_cmp++;
        }
        ans += ((ans_t)cur_fifth) * (fourth_root_size - 1) - common_size;
    }
    return ans;
}

static ans_t __imp_house5(sysname_t tasklet_id, node_t root) {
    edge_ptr root_begin = row_ptr[root];  // intended DMA
    edge_ptr root_end = row_ptr[root + 1];  // intended DMA
    ans_t ans = 0;
    for (edge_ptr i = root_begin; i < root_end; i++) {
        node_t second_root = col_idx[i];  // intended DMA
        if (second_root >= root) break;
        ans += __imp_house5_2(tasklet_id, root, second_root, 0, 1);
    }
    return ans;
}

extern void house5(sysname_t tasklet_id) {
    static ans_t partial_ans[NR_TASKLETS];
    static uint64_t partial_cycle[NR_TASKLETS];
    static perfcounter_cycles cycles[NR_TASKLETS];

    node_t i = 0;
    while (i < root_num) {
        node_t root = roots[i];  // intended DMA
        node_t root_begin = row_ptr[root];  // intended DMA
        node_t root_end = row_ptr[root + 1];  // intended DMA

        // the same partition condition for depth 1 and 2, so direct partition at depth 2
        if (root_end - root_begin < BRANCH_LEVEL_THRESHOLD) {
            break;
        }

        barrier_wait(&co_barrier);
#ifdef PERF
        timer_start(&cycles[tasklet_id]);
#endif
        partial_ans[tasklet_id] = 0;
        for (edge_ptr j = root_begin; j < root_end; j++) {
            node_t second_root = col_idx[j];  // intended DMA
            if (second_root >= root) break;
            partial_ans[tasklet_id] += __imp_house5_2(tasklet_id, root, second_root, tasklet_id, NR_TASKLETS);
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
        ans[i] = __imp_house5(tasklet_id, root);  // intended DMA
#ifdef PERF
        cycle_ct[i] = timer_stop(&cycles[tasklet_id]);  // intended DMA
#endif
    }
}
