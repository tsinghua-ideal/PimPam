#include <common.h>
#include <cyclecount.h>
#include <string.h>
#include <mram.h>
#include <defs.h>
#include <assert.h>
#include <barrier.h>

// transferred data
__mram_noinit edge_ptr row_ptr[DPU_N];   // 8M
__mram_noinit node_t col_idx[DPU_M];    // 32M
__host uint64_t root_num;
__mram_noinit node_t roots[DPU_ROOT_NUM];   // 1M
__mram_noinit uint64_t ans[DPU_ROOT_NUM];   // 2M
__mram_noinit uint64_t cycle_ct[DPU_ROOT_NUM];   // 2M

// buffer
node_t buf[NR_TASKLETS][3][BUF_SIZE];  // 6K
__mram_noinit node_t mram_buf[NR_TASKLETS << 2][MRAM_BUF_SIZE];  // <=16M
#ifdef BITMAP
uint32_t bitmap_size;
uint32_t bitmap[NR_TASKLETS * 3][BITMAP_SIZE];  // 12K
__mram_noinit uint32_t mram_bitmap[BITMAP_SIZE << 5][BITMAP_SIZE];  // 256K
#endif

// synchronization
BARRIER_INIT(co_barrier, NR_TASKLETS);

// intersection
extern node_t intersect_seq_buf_thresh(node_t(*buf)[BUF_SIZE], node_t __mram_ptr *a, node_t a_size, node_t __mram_ptr *b, node_t b_size, node_t __mram_ptr *c, node_t threshold);

#ifdef BITMAP
extern void intersect_bitmap(node_t *a, node_t *b, node_t *c, node_t bitmap_size);

void build_bitmap(node_t root, edge_ptr root_begin, edge_ptr root_end, sysname_t tasklet_id) {
    static uint32_t thread_bitmap_size[NR_TASKLETS];
    node_t *a_buf = buf[tasklet_id][0];
    node_t *b_buf = buf[tasklet_id][1];

    node_t root_size = root_end - root_begin;
    thread_bitmap_size[tasklet_id] = 0;
    for (node_t cur = tasklet_id; cur < root_size; cur += NR_TASKLETS) {
        node_t neighbor = col_idx[root_begin + cur];  // intended DMA
        if (neighbor >= root) {
            break;
        }
        node_t neighbor_begin = row_ptr[neighbor];  // intended DMA
        node_t neighbor_end = row_ptr[neighbor + 1];  // intended DMA
        memset(bitmap[tasklet_id], 0, sizeof(bitmap[tasklet_id]));

        node_t __mram_ptr *a = &col_idx[root_begin];
        node_t a_size = root_end - root_begin;
        node_t __mram_ptr *b = &col_idx[neighbor_begin];
        node_t b_size = neighbor_end - neighbor_begin;
        node_t i = 0, j = 0, k = 0;
        if (((uint64_t)a) & 4) {
            a--;
            i = 1;
            a_size++;
        }
        if (((uint64_t)b) & 4) {
            b--;
            j = 1;
            b_size++;
        }
        mram_read(a, a_buf, ALIGN8(MIN(a_size, BUF_SIZE) << SIZE_NODE_T_LOG));
        mram_read(b, b_buf, ALIGN8(MIN(b_size, BUF_SIZE) << SIZE_NODE_T_LOG));

        while (i < a_size && j < b_size) {
            if (i == BUF_SIZE) {
                a_size -= i;
                a += i;
                mram_read(a, a_buf, ALIGN8(MIN(a_size, BUF_SIZE) << SIZE_NODE_T_LOG));
                i = 0;
            }
            if (j == BUF_SIZE) {
                b_size -= j;
                b += j;
                mram_read(b, b_buf, ALIGN8(MIN(b_size, BUF_SIZE) << SIZE_NODE_T_LOG));
                j = 0;
            }

            if (a_buf[i] >= neighbor || b_buf[j] >= neighbor) break;

            if (a_buf[i] == b_buf[j]) {
                bitmap[tasklet_id][k >> 5] |= 1 << (k & 31);
                i++;
                k++;
                j++;
            }
            else if (a_buf[i] < b_buf[j]) {
                i++;
                k++;
            }
            else {
                j++;
            }
        }
        mram_write(bitmap[tasklet_id], mram_bitmap[cur], sizeof(bitmap[tasklet_id]));
        uint32_t new_bitmap_size = (k >> 5) + 1;
        if (new_bitmap_size > thread_bitmap_size[tasklet_id]) thread_bitmap_size[tasklet_id] = new_bitmap_size;
    }
    barrier_wait(&co_barrier);
    if (tasklet_id == 0) {
        bitmap_size = 0;
        for (uint32_t i = 0; i < NR_TASKLETS; i++) {
            if (thread_bitmap_size[i] > bitmap_size) bitmap_size = thread_bitmap_size[i];
        }
    }
}
#endif
