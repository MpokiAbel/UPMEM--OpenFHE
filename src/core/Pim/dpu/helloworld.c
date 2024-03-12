#include <perfcounter.h>
#include <stdio.h>

__host uint32_t nb_cycles;

int main() {
    perfcounter_config(COUNT_CYCLES, true);

    int loop = 1e7;
    while (loop)
        loop--;

    nb_cycles = perfcounter_get();

    return 0;
}