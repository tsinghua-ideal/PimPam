#include <common.h>
#include <stdint.h>
#include <mram.h>
#include <defs.h>
#include <alloc.h>
#include <mutex.h>
#include <barrier.h>

__mram_noinit_keep uint32_t bitmap[N >> 5];
__mram_noinit_keep uint32_t involve_bitmap[N >> 5];
__mram_noinit_keep uint32_t renumber[N];
__mram_noinit_keep edge_ptr row_ptr[PARTITION_M];
__mram_noinit_keep node_t col_idx[PARTITION_M];
__mram_noinit_keep edge_ptr processed_row_ptr[PARTITION_M];
__mram_noinit_keep node_t processed_col_idx[PARTITION_M];
__mram_noinit_keep node_t roots[DPU_ROOT_NUM];
__host uint64_t start;
__host uint64_t size;
__host uint64_t root_size;
__host uint64_t mode;   // 0 for first round, 1 for renumber, 2 for second round
__host uint64_t processed_row_size;
__host uint64_t processed_col_size;
__host uint64_t processed_offset;

MUTEX_INIT(latch);
BARRIER_INIT(barrier, NR_TASKLETS);

int main() {
    sysname_t tasklet_id = me();
    if (tasklet_id == 0) {
        mem_reset();
    }

    if (mode == 0) {
        uint32_t cur_bitmap = 0;
        edge_ptr offset = row_ptr[0];
        for (node_t i = tasklet_id; i < size; i += NR_TASKLETS) {
            cur_bitmap = bitmap[(start + i) >> 5];   // intended DMA
            if (cur_bitmap & (1 << ((start + i) & 31))) {
                mutex_lock(latch);
                involve_bitmap[(start + i) >> 5] |= (1 << ((start + i) & 31));   // intended DMA
                mutex_unlock(latch);
                edge_ptr node_begin = row_ptr[i] - offset;   // intended DMA
                edge_ptr node_end = row_ptr[i + 1] - offset;   // intended DMA
                for (edge_ptr j = node_begin; j < node_end; j++) {
                    node_t neighbor = col_idx[j];   // intended DMA  TODO: optimize
                    mutex_lock(latch);
                    involve_bitmap[neighbor >> 5] |= (1 << (neighbor & 31));   // intended DMA
                    mutex_unlock(latch);
                }
            }
        }
    }
    else if (mode == 1) {
        if (tasklet_id != 0) return 0;
        node_t cur = 0;
        node_t n = (size + 31) >> 5;
        for (node_t i = 0; i < n; i++) {
            uint32_t cur_bitmap = involve_bitmap[i];   // intended DMA
            for (node_t j = 0; j < 32; j++) {
                if (cur_bitmap & (1 << j)) {
                    renumber[(i << 5) | j] = cur;
                    cur++;
                }
            }
        }
        for (node_t i = 0; i < root_size; i++) {
            roots[i] = renumber[roots[i]];   // intended DMA
        }
    }
    else if (mode == 2) {
        if (tasklet_id != 0) return 0;
        uint32_t cur_bitmap = 0;
        uint32_t cur_involve_bitmap = 0;
        edge_ptr offset = row_ptr[0];
        uint64_t row_size = 0;
        uint64_t col_size = 0;
        uint32_t need_fetch = 1;
        for (node_t i = 0; i < size; i++) {
            if (need_fetch) {
                cur_bitmap = bitmap[(start + i) >> 5];   // intended DMA
                cur_involve_bitmap = involve_bitmap[(start + i) >> 5];   // intended DMA
                need_fetch = 0;
            }
            if (cur_involve_bitmap & (1 << ((start + i) & 31))) {
                processed_row_ptr[row_size] = col_size + processed_offset;   // intended DMA
                row_size++;
                if (cur_bitmap & (1 << ((start + i) & 31))) {
                    edge_ptr node_begin = row_ptr[i] - offset;   // intended DMA
                    edge_ptr node_end = row_ptr[i + 1] - offset;   // intended DMA
                    for (edge_ptr j = node_begin; j < node_end; j++) {
                        node_t new_idx = renumber[col_idx[j]];   // intended DMA
                        processed_col_idx[col_size] = new_idx;   // intended DMA
                        col_size++;
                    }
                }
            }
            if (((start + i) & 31) == 31) {
                need_fetch = 1;
            }
        }
        processed_row_size = row_size;
        processed_col_size = col_size;
    }
    return 0;
}