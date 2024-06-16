#include <common.h>
#include <stdint.h>
#include <assert.h>
#include <mram.h>
#include <defs.h>
#include <alloc.h>
#include <barrier.h>
#include <perfcounter.h>

extern void KERNEL_FUNC(sysname_t tasklet_id);

BARRIER_INIT(my_barrier, NR_TASKLETS);

int main() {
	sysname_t tasklet_id = me();
	if (tasklet_id == 0) {
		mem_reset();
#ifdef DPU_LOG
		printf("just for log\n");
#endif
#ifdef PERF
		perfcounter_config(COUNT_CYCLES, true);
#endif
	}
	barrier_wait(&my_barrier);

	KERNEL_FUNC(tasklet_id);
	return 0;
}