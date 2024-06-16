#include <common.h>

typedef struct ElementType {
    uint32_t dpu_id;
    double workload;
} ElementType;

uint32_t queue_size;
ElementType Elements[NR_DPUS];

void queue_init() {
    for (uint32_t i = 0; i < NR_DPUS; i++) {
        Elements[i].dpu_id = i;
        Elements[i].workload = 0;
    }
    queue_size = NR_DPUS;
}

void push_to_queue(uint32_t dpu_id, double workload) {
    Elements[queue_size].dpu_id = dpu_id;
    Elements[queue_size].workload = workload;
    for (uint32_t i = queue_size++; i; i = (i - 1) >> 1) {
        uint32_t j = (i - 1) >> 1;
        if(Elements[i].workload < Elements[j].workload || (Elements[i].workload == Elements[j].workload && Elements[i].dpu_id < Elements[j].dpu_id)) {
            ElementType tmp = Elements[i];
            Elements[i] = Elements[j];
            Elements[j] = tmp;
        }
        else break;
    }
}

uint32_t pop_from_queue() {
    uint32_t ans = Elements[0].dpu_id;
    Elements[0] = Elements[--queue_size];
    uint32_t i = 0;
    while (1) {
        uint32_t left = (i << 1) + 1;
        uint32_t right = (i << 1) + 2;
        if (left >= queue_size) break;
        uint32_t min_element = i;
        if (Elements[left].workload < Elements[min_element].workload || (Elements[left].workload == Elements[min_element].workload && Elements[left].dpu_id < Elements[min_element].dpu_id)) {
            min_element = left;
        }
        if (right < queue_size && (Elements[right].workload < Elements[min_element].workload || (Elements[right].workload == Elements[min_element].workload && Elements[right].dpu_id < Elements[min_element].dpu_id))) {
            min_element = right;
        }
        if (min_element == i) break;
        ElementType tmp = Elements[i];
        Elements[i] = Elements[min_element];
        Elements[min_element] = tmp;
        i = min_element;
    }
    return ans;
}