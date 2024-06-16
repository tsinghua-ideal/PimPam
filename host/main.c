#include <common.h>
#include <timer.h>
#include <assert.h>
#include <stdio.h>
#include <dpu.h>

extern void data_transfer(struct dpu_set_t set, Graph *g);
extern ans_t clique2(Graph *g, node_t root);
extern ans_t KERNEL_FUNC(Graph *g, node_t root);
Graph *g;
ans_t ans[N];
ans_t result[N];
Timer timer;
uint64_t cycle_ct[N];
uint64_t cycle_ct_dpu[NR_DPUS][NR_TASKLETS];

int main() {
    printf("NR_DPUS: %u, NR_TASKLETS: %u, DPU_BINARY: %s, PATTERN: %s\n", NR_DPUS, NR_TASKLETS, DPU_BINARY, PATTERN_NAME);

    struct dpu_set_t set, dpu;
    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &set));

    // task allocation and data partition
    printf("Selecting graph: %s\n", DATA_PATH);
    start(&timer, 0, 0);
    g = malloc(sizeof(Graph));
    data_transfer(set, g);
    stop(&timer, 0);
    printf("Data transfer ");
    print(&timer, 0, 1);

    // run it on CPU to get the answer
    ans_t total_ans = 0;
#ifdef CPU_RUN
    start(&timer, 0, 0);
    for (node_t i = 0; i < g->n; i++) {
        ans[i] = KERNEL_FUNC(g, i);
        total_ans += ans[i];
    }
    stop(&timer, 0);
    printf("CPU ");
    print(&timer, 0, 1);
    printf("CPU ans: %lu\n", total_ans);
#endif  // CPU_RUN

    // run it on DPU
    start(&timer, 0, 0);
    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
    stop(&timer, 0);
    printf("DPU ");
    print(&timer, 0, 1);

    // collect answer and cycle count
    bool fine = true;
    bool finished, failed;
    uint32_t each_dpu;
    total_ans = 0;
#ifdef PERF
    uint64_t total_cycle_ct = 0;
#endif
    DPU_FOREACH(set, dpu, each_dpu) {
        // check status
        DPU_ASSERT(dpu_status(dpu, &finished, &failed));
        if (failed) {
            printf("DPU: %u failed\n", each_dpu);
            fine = false;
            break;
        }

        // collect answer
        uint64_t *dpu_ans = (uint64_t *)malloc(ALIGN8(g->root_num[each_dpu] * sizeof(uint64_t)));
        DPU_ASSERT(dpu_copy_from(dpu, "ans", 0, dpu_ans, ALIGN8(g->root_num[each_dpu] * sizeof(uint64_t))));
        for (node_t k = 0; k < g->root_num[each_dpu]; k++) {
            node_t cur_root = g->roots[each_dpu][k];
            result[cur_root] = dpu_ans[k];
            total_ans += dpu_ans[k];
#ifdef CPU_RUN
            if (ans[cur_root] != dpu_ans[k]) {
                printf("Wrong answer at dpu %u node %u: %lu != %lu\n", each_dpu, cur_root, ans[cur_root], dpu_ans[k]);
#ifdef DPU_LOG
                DPU_ASSERT(dpu_log_read(dpu, stdout));
#endif
                fine = false;
            }
#endif  // CPU_RUN
        }
        free(dpu_ans);

        // collect cycle count
#ifdef PERF
        uint64_t *dpu_cycle_ct = (uint64_t *)malloc(ALIGN8(g->root_num[each_dpu] * sizeof(uint64_t)));
        DPU_ASSERT(dpu_copy_from(dpu, "cycle_ct", 0, dpu_cycle_ct, ALIGN8(g->root_num[each_dpu] * sizeof(uint64_t))));
        for (node_t k = 0, cur_thread = 0; k < g->root_num[each_dpu]; k++) {
            node_t cur_root = g->roots[each_dpu][k];
            cycle_ct[cur_root] = dpu_cycle_ct[k];
            if (g->row_ptr[cur_root + 1] - g->row_ptr[cur_root] >= BRANCH_LEVEL_THRESHOLD) {
                for (uint32_t i = 0; i < NR_TASKLETS; i++) {
                    cycle_ct_dpu[each_dpu][i] += dpu_cycle_ct[k] / NR_TASKLETS;
                }
                total_cycle_ct += dpu_cycle_ct[k];
            }
            else {
                cycle_ct_dpu[each_dpu][cur_thread] += dpu_cycle_ct[k];
                cur_thread = (cur_thread + 1) % NR_TASKLETS;
                total_cycle_ct += dpu_cycle_ct[k];
            }
        }
        free(dpu_cycle_ct);
#endif
    }
    printf("DPU ans: %lu\n", total_ans);
#ifdef PERF
    printf("Lower bound: %f\n", (double)total_cycle_ct / NR_DPUS / NR_TASKLETS / 350000);
#endif

    // output result to file
#ifdef PERF
    FILE *fp = fopen("./result/" PATTERN_NAME "_" DATA_NAME ".txt", "w");
    fprintf(fp, "NR_DPUS: %u, NR_TASKLETS: %u, DPU_BINARY: %s, PATTERN: %s\n", NR_DPUS, NR_TASKLETS, DPU_BINARY, PATTERN_NAME);
    fprintf(fp, "N: %u, M: %u, avg_deg: %f\n", g->n, g->m, (double)g->m / g->n);
    for (node_t i = 0; i < g->n; i++) {
        fprintf(fp, "node: %u, deg: %u, o_deg: %lu, ans: %lu, cycle: %lu,\n", i, g->row_ptr[i + 1] - g->row_ptr[i], clique2(g, i), result[i], cycle_ct[i]);
    }
    for (uint32_t i = 0; i < NR_DPUS; i++) {
        for (uint32_t j = 0; j < NR_TASKLETS; j++) {
            fprintf(fp, "DPU: %u, tasklet: %u, cycle: %lu, root_num: %lu\n", i, j, cycle_ct_dpu[i][j], g->root_num[i]);
        }
    }
    fclose(fp);
#endif
    if (fine) printf(ANSI_COLOR_GREEN "All fine\n" ANSI_COLOR_RESET);
    else printf(ANSI_COLOR_RED "Some failed\n" ANSI_COLOR_RESET);

    free(g);
    DPU_ASSERT(dpu_free(set));
    return 0;
}