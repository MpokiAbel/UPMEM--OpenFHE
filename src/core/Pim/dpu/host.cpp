#include <chrono>
#include <dpu>
#include <iostream>

#ifndef DPU_BINARY
    #define DPU_BINARY "./helloworld"
#endif

using namespace dpu;

int main() {
    try {
        auto dpu = DpuSet::allocate(1);
        std::cout << "DPU allocated" << std::endl;
        dpu.load(DPU_BINARY);

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        dpu.exec();
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

        // retrieve number of cycles on DPU
        std::vector<std::vector<uint32_t>> nbCycles(1);
        nbCycles.front().resize(1);
        dpu.copy(nbCycles, "nb_cycles");

        // retrieve DPU frequency
        std::vector<std::vector<uint32_t>> clocksPerSec(1);
        clocksPerSec.front().resize(1);
        dpu.copy(clocksPerSec, "CLOCKS_PER_SEC");

        std::cout.precision(2);
        std::cout << std::scientific << "DPU cycles: " << nbCycles.front().front() << std::endl
                  << "DPU time: " << (double)nbCycles.front().front() / clocksPerSec.front().front() << " secs."
                  << std::endl;

        std::cout << "Host elapsed time: "
                  << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1.0e9 << " secs."
                  << std::endl;
    }
    catch (const DpuError& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}