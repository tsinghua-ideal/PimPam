#include <dpu_mine.h>

static ans_t __imp_clique2(sysname_t tasklet_id, node_t root) {
    edge_ptr root_begin = row_ptr[root];  // intended DMA
    edge_ptr root_end = row_ptr[root + 1];  // intended DMA
    ans_t ans = 0;
    node_t *tasklet_buf = buf[tasklet_id][0];

    node_t j = 0;
    if (root_begin & 1) {  // special case: root_begin is odd
        root_begin--;
        j = 1;
    }
    for (edge_ptr i = root_begin; i < root_end; i += BUF_SIZE) {
        node_t transfer_size = BUF_SIZE < root_end - i ? BUF_SIZE : root_end - i;
        mram_read(&col_idx[i], tasklet_buf, ALIGN8(transfer_size * sizeof(node_t)));
        for (; j < transfer_size; j++) {
            if (tasklet_buf[j] >= root) return ans;
            ans++;
        }
        j = 0;
    }
    return ans;
}

extern void clique2(sysname_t tasklet_id) {
    static perfcounter_cycles cycles[NR_TASKLETS];
    
    for (node_t i = tasklet_id; i < root_num; i += NR_TASKLETS) {
        node_t root = roots[i];  // intended DMA
#ifdef PERF
        timer_start(&cycles[tasklet_id]);
#endif
        ans[i] = __imp_clique2(tasklet_id, root);  // intended DMA
#ifdef PERF
        cycle_ct[i] = timer_stop(&cycles[tasklet_id]);  // intended DMA
#endif
    }
}
